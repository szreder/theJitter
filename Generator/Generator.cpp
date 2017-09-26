#include <optional>
#include <type_traits>
#include <variant>

#include "Generator/AST.hpp"
#include "Generator/Generator.hpp"
#include "Generator/Program.hpp"
#include "Generator/Runtime.hpp"
#include "Generator/RValue.hpp"
#include "Util/Casts.hpp"
#include "Util/PrettyPrint.hpp"

namespace {

void runcall(Program &program, gcc_jit_function *func, gcc_jit_block *block, int call, void *arg)
{
	std::cout << "Generating runcall " << call << " with arg addr = " << arg << '\n';
	auto ctx = program.context();
	gcc_jit_rvalue *call_params[2] = {
		gcc_jit_context_new_rvalue_from_int(ctx, program.type(ValueType::Integer), call),
		gcc_jit_context_new_rvalue_from_ptr(ctx, program.type(ValueType::Unknown), arg),
	};
	gcc_jit_rvalue *jitCall = gcc_jit_context_new_call_through_ptr(ctx, nullptr, program.runtimeCallPtr(), 2, call_params);
	gcc_jit_block_add_eval(block, nullptr, jitCall);
}

typedef std::optional <RValue> GenResult;

GenResult dispatch(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src);

void checkType(const RValue &rvalue, const Node *n)
{
	if (rvalue.valueType() == ValueType::Invalid) {
		std::cerr << "Invalid type";
		n->print();
		abort();
	}
}

template <typename T>
GenResult generateImmediateSpec(const RValue &opLeft, const RValue &opRight, BinOp::Type op);

template <>
GenResult generateImmediateSpec<std::string>(const RValue &opLeft, const RValue &opRight, BinOp::Type op)
{
	if (op == BinOp::Type::Plus)
		return RValue{opLeft.value<std::string>() + opRight.value<std::string>()};

	abort();
	return {};
}

template <typename T>
GenResult generateImmediateArithm(const RValue &opLeft, const RValue &opRight, BinOp::Type op)
{
	return RValue::executeBinOp<T>(opLeft, opRight, op);
}

template <typename T>
GenResult generateImmediate(const RValue &opLeft, const RValue &opRight, BinOp::Type op)
{
	if constexpr(std::is_arithmetic<T>::value)
		return generateImmediateArithm<T>(opLeft, opRight, op);

	return generateImmediateSpec<T>(opLeft, opRight, op);
}

GenResult generateImmediate(RValue &opLeft, RValue &opRight, BinOp::Type op)
{
	matchTypes(opLeft, opRight);
	if (!BinOp::isApplicable(op, opLeft.valueType())) {
		std::cerr << "Binary operation " << BinOp::toString(op) << " not possible for type " << prettyPrint(opLeft.valueType()) << '\n';
		abort();
	}

	switch (opLeft.valueType()) {
		case ValueType::Boolean:
			return generateImmediate<bool>(opLeft, opRight, op);
		case ValueType::Integer:
			return generateImmediate<int>(opLeft, opRight, op);
		case ValueType::Real:
			return generateImmediate<double>(opLeft, opRight, op);
		case ValueType::String:
			return generateImmediate<std::string>(opLeft, opRight, op);
	}

	assert(false);
	return {};
}

template <Node::Type type>
GenResult generate(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src);

GenResult generate(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src);

std::vector <GenResult> generateExprList(Program &program, gcc_jit_function *func, gcc_jit_block *block, const ExprList *exprList)
{
	std::vector <GenResult> exprResults(exprList->exprs().size());
	size_t i = 0;

	for (const Node *expr : exprList->exprs()) {
		exprResults[i] = dispatch(program,func, block, expr);
		if (!exprResults[i])
			exprResults[i] = RValue::Nil();
		++i;
	}
	return exprResults;
}

template <>
GenResult generate<Node::Type::Assignment>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const Assignment *c = static_cast<const Assignment *>(src);
	const ExprList *exprList = c->exprList();
	std::vector <GenResult> exprResults = generateExprList(program, func, block, exprList);

	size_t i = 0;
	const VarList *varList = c->varList();
	for (const std::string &var : varList->vars()) {
		runcall(program, func, block, RUNCALL_PUSH, program.duplicateString(var));
		if (i >= exprResults.size() || !exprResults[i]) {
			runcall(program, func, block, RUNCALL_VARIABLE_UNSET, nullptr);
		} else {
			runcall(program, func, block, RUNCALL_PUSH, program.allocRValue(exprResults[i].value()));
			runcall(program, func, block, RUNCALL_VARIABLE_SET, nullptr);
		}
		++i;
	}

	return {};
}

template <>
GenResult generate<Node::Type::BinOp>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	static int binopVarCnt = 0;
	char buff[30];

	const BinOp *bo = static_cast<const BinOp *>(src);

	GenResult left = dispatch(program, func, block, bo->left());
	RValue &leftRValue = left.value();
	checkType(leftRValue, bo->left());

	GenResult right = dispatch(program, func, block, bo->right());
	RValue &rightRValue = right.value();
	checkType(rightRValue, bo->right());

	if (leftRValue.type() == RValue::Type::Immediate && rightRValue.type() == RValue::Type::Immediate)
		return generateImmediate(leftRValue, rightRValue, bo->binOpType());

	sprintf(buff, "__binop_v_%d", ++binopVarCnt);
	gcc_jit_lvalue *binopTmp = gcc_jit_function_new_local(func, nullptr, program.type(ValueType::Unknown), buff);

	runcall(program, func, block, RUNCALL_PUSH, program.duplicateString(buff));
	runcall(program, func, block, RUNCALL_PUSH, program.allocRValue(rightRValue));
	runcall(program, func, block, RUNCALL_PUSH, program.allocRValue(leftRValue));
	runcall(program, func, block, RUNCALL_BINOP, toVoidPtr(bo->binOpType()));

	return RValue::asVariable(std::string{buff});
}

template <>
GenResult generate<Node::Type::Chunk>(Program &program, gcc_jit_function *func, gcc_jit_block *, const Node *src)
{
	const Chunk *c = static_cast<const Chunk *>(src);
	gcc_jit_block *block = gcc_jit_function_new_block(func, nullptr);
	runcall(program, func, block, RUNCALL_SCOPE_PUSH, nullptr);

	for (const Node *n : c->children())
		dispatch(program, func, block, n);

	gcc_jit_block_end_with_void_return(block, nullptr);
	return {};
}

template <>
GenResult generate<Node::Type::FunctionCall>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	static int funcArgCnt = 0;
	char buff[30];

	const FunctionCall *f = static_cast<const FunctionCall *>(src);
	const ExprList *args = f->args();
	std::vector <GenResult> exprResults = generateExprList(program, func, block, args);

	for (auto i = exprResults.crbegin(); i != exprResults.crend(); ++i)
		runcall(program, func, block, RUNCALL_PUSH, program.allocRValue(i->value()));
	runcall(program, func, block, RUNCALL_PUSH, toVoidPtr(args->exprs().size()));
	runcall(program, func, block, RUNCALL_PUSH, program.allocRValue(f->functionExpr()));
	runcall(program, func, block, RUNCALL_FUNCTION_CALL, nullptr);
}

template <>
GenResult generate<Node::Type::Value>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const Value *v = static_cast<const Value *>(src);

	switch (v->valueType()) {
		case ValueType::Nil:
			return RValue::Nil();
		case ValueType::Boolean:
			return RValue{static_cast<const BooleanValue *>(v)->value()};
		case ValueType::Integer:
			return RValue{static_cast<const IntValue *>(v)->value()};
		case ValueType::Real:
			return RValue{static_cast<const RealValue *>(v)->value()};
		case ValueType::String:
			return RValue{static_cast<const StringValue *>(v)->value()};
	}

	std::cerr << "generate<Node::Type::Value>() not implemented for value type: " << prettyPrint(v->valueType()) << '\n';
	abort();
	return {};
}

template <>
GenResult generate<Node::Type::Variable>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const VarNode *v = static_cast<const VarNode *>(src);
	return RValue::asVariable(v->name());
}

GenResult dispatch(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	switch (src->type()) {
		case Node::Type::Assignment:
			return generate<Node::Type::Assignment>(program, func, block, src);
		case Node::Type::BinOp:
			return generate<Node::Type::BinOp>(program, func, block, src);
		case Node::Type::Chunk:
			return generate<Node::Type::Chunk>(program, func, block, src);
		case Node::Type::FunctionCall:
			return generate<Node::Type::FunctionCall>(program, func, block, src);
		case Node::Type::Value:
			return generate<Node::Type::Value>(program, func, block, src);
		case Node::Type::Variable:
			return generate<Node::Type::Variable>(program, func, block, src);
	}

	std::cerr << "generate() not implemented for type: " << prettyPrint(src->type()) << '\n';
	abort();
	return {};
}

} //namespace

void generate(const Node *root)
{
	if (root == nullptr) {
		std::cerr << "Empty code\n";
		return;
	}

	if (root->type() != Node::Type::Chunk) {
		std::cerr << "Root node should be a Chunk\n";
		return;
	}

	Program &program = Program::getInstance();
	dispatch(program, program.main(), nullptr, root);
}
