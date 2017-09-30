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

namespace Lua {

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

#define RUNCALL(call, arg) \
	runcall(program, func, block, (call), (arg))

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

template <typename T>
GenResult generateImmediate(const RValue &operand, UnOp::Type op)
{
	return RValue::executeUnOp<T>(operand, op);
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
		default:
			break;
	}

	assert(false);
	return {};
}

GenResult generateImmediate(RValue &operand, UnOp::Type op)
{
	switch (operand.valueType()) {
		case ValueType::Boolean:
			return generateImmediate<bool>(operand, op);
		case ValueType::Integer:
			return generateImmediate<int>(operand, op);
		case ValueType::Real:
			return generateImmediate<double>(operand, op);
		default:
			break;
	}

	assert(false);
	return {};
}

template <Node::Type type>
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
	for (const LValue *lval : varList->vars()) {
		switch (lval->lvalueType()) {
			case LValue::Type::Name:
				RUNCALL(RUNCALL_PUSH, program.duplicateString(lval->name()));
				if (i >= exprResults.size() || !exprResults[i]) {
					RUNCALL(RUNCALL_VARIABLE_UNSET, nullptr);
				} else {
					RUNCALL(RUNCALL_PUSH, program.allocRValue(exprResults[i].value()));
					RUNCALL(RUNCALL_VARIABLE_SET, nullptr);
				}
				++i;
				break;
			case LValue::Type::Expression:
				std::cerr << "Not implemented yet\n";
				abort();
				break;
		}
	}

	return {};
}

template <>
GenResult generate<Node::Type::BinOp>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	static int binOpVarCnt = 0;
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

	sprintf(buff, "__binOp_v_%d", ++binOpVarCnt);
	gcc_jit_function_new_local(func, nullptr, program.type(ValueType::Unknown), buff);

	RUNCALL(RUNCALL_PUSH, program.duplicateString(buff));
	RUNCALL(RUNCALL_PUSH, program.allocRValue(rightRValue));
	RUNCALL(RUNCALL_PUSH, program.allocRValue(leftRValue));
	RUNCALL(RUNCALL_BINOP, toVoidPtr(bo->binOpType()));

	return RValue::asVariable(std::string{buff});
}

template <>
GenResult generate<Node::Type::UnOp>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	static int unOpVarCnt = 0;
	char buff[30];

	const UnOp *uo = static_cast<const UnOp *>(src);
	GenResult sub = dispatch(program, func, block, uo->operand());
	RValue &val = sub.value();
	checkType(val, uo->operand());

	if (val.type() == RValue::Type::Immediate)
		return generateImmediate(val, uo->unOpType());

	sprintf(buff, "__unOp_v_%d", ++unOpVarCnt);
	gcc_jit_function_new_local(func, nullptr, program.type(ValueType::Unknown), buff);

	RUNCALL(RUNCALL_PUSH, program.duplicateString(buff));
	RUNCALL(RUNCALL_PUSH, program.allocRValue(val));
	RUNCALL(RUNCALL_UNOP, toVoidPtr(uo->unOpType()));

	return RValue::asVariable(std::string{buff});
}

template <>
GenResult generate<Node::Type::Chunk>(Program &program, gcc_jit_function *func, gcc_jit_block *, const Node *src)
{
	const Chunk *c = static_cast<const Chunk *>(src);
	gcc_jit_block *block = gcc_jit_function_new_block(func, nullptr);
	RUNCALL(RUNCALL_SCOPE_PUSH, nullptr);

	for (const Node *n : c->children())
		dispatch(program, func, block, n);

	gcc_jit_block_end_with_void_return(block, nullptr);
	return {};
}

template <>
GenResult generate<Node::Type::FunctionCall>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	static int funcResCnt = 0;
	char buff[30];

	const FunctionCall *f = static_cast<const FunctionCall *>(src);
	GenResult funcResolved = dispatch(program, func, block, f->functionExpr());
	if (!funcResolved) {
		std::cerr << "Unable to resolve function call:\n";
		f->functionExpr()->print();
		abort();
	}

	const ExprList *args = f->args();
	std::vector <GenResult> exprResults = generateExprList(program, func, block, args);

	sprintf(buff, "__fn_res_v_%d", ++funcResCnt);
	gcc_jit_function_new_local(func, nullptr, program.type(ValueType::Unknown), buff);

	RUNCALL(RUNCALL_PUSH, program.duplicateString(buff));
	for (auto i = exprResults.crbegin(); i != exprResults.crend(); ++i)
		RUNCALL(RUNCALL_PUSH, program.allocRValue(i->value()));
	RUNCALL(RUNCALL_PUSH, toVoidPtr(args->exprs().size()));
	RUNCALL(RUNCALL_PUSH, program.allocRValue(*funcResolved));
	RUNCALL(RUNCALL_FUNCTION_CALL, nullptr);

	return RValue::asVariable(std::string{buff});
}

template <>
GenResult generate<Node::Type::TableCtor>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const TableCtor *tv = static_cast<const TableCtor *>(src);

	static int tableVarCnt = 0;
	char buff[30];

	sprintf(buff, "__table_v_%d", ++tableVarCnt);
	gcc_jit_function_new_local(func, nullptr, program.type(ValueType::Unknown), buff);
	RUNCALL(RUNCALL_PUSH, program.duplicateString(buff));

	auto fields = tv->fields();

	int fieldCounter = 0;
	for (const auto &field : fields) {
		if (field->fieldType() == Field::Type::NoIndex) {
			RUNCALL(RUNCALL_PUSH, program.allocRValue(*dispatch(program, func, block, field->valueExpr())));
			RUNCALL(RUNCALL_PUSH, program.allocRValue(RValue{++fieldCounter}));
		}
	}

	for (const auto &field : fields) {
		if (field->fieldType() != Field::Type::NoIndex) {
			RValue index;
			switch (field->fieldType()) {
				case Field::Type::Brackets:
					index = *dispatch(program, func, block, field->keyExpr());
					break;
				case Field::Type::Literal:
					index = RValue{field->fieldName()};
					break;
				default:
					break;
			}
			RUNCALL(RUNCALL_PUSH, program.allocRValue(*dispatch(program, func, block, field->valueExpr())));
			RUNCALL(RUNCALL_PUSH, program.allocRValue(index));
		}
	}

	RUNCALL(RUNCALL_PUSH, toVoidPtr(fields.size()));
	RUNCALL(RUNCALL_TABLE_CTOR, nullptr);

	return RValue::asVariable(std::string{buff});
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
		default:
			break;
	}

	std::cerr << "generate<Node::Type::Value>() not implemented for value type: " << prettyPrint(v->valueType()) << '\n';
	abort();
	return {};
}

template <>
GenResult generate<Node::Type::LValue>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const LValue *lval = static_cast<const LValue *>(src);
	if (lval->lvalueType() == Lua::LValue::Type::Name)
		return RValue::fromLValue(lval);

	return {};
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
		case Node::Type::TableCtor:
			return generate<Node::Type::TableCtor>(program, func, block, src);
		case Node::Type::UnOp:
			return generate<Node::Type::UnOp>(program, func, block, src);
		case Node::Type::Value:
			return generate<Node::Type::Value>(program, func, block, src);
		case Node::Type::LValue:
			return generate<Node::Type::LValue>(program, func, block, src);
		default:
			break;
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

} //namespace Lua
