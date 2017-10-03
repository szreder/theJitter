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

template <Node::Type type>
RValue * generate(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src);

RValue * dispatch(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src);

void checkType(const RValue *rvalue, const Node *n)
{
	if (rvalue->type() == RValue::Type::Immediate && rvalue->valueType() == ValueType::Invalid) {
		std::cerr << "Invalid type\n";
		n->print();
		abort();
	}
}

std::vector <RValue *> generateExprList(Program &program, gcc_jit_function *func, gcc_jit_block *block, const ExprList *exprList)
{
	std::vector <RValue *> exprResults(exprList->exprs().size());
	size_t i = 0;

	for (const auto &expr : exprList->exprs()) {
		exprResults[i] = dispatch(program, func, block, expr.get());
		if (exprResults[i] == nullptr)
			exprResults[i] = program.allocRValue(RValue::Nil());
		++i;
	}
	return exprResults;
}

template <>
RValue * generate<Node::Type::FunctionCall>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const FunctionCall *f = static_cast<const FunctionCall *>(src);
	RValue *funcResolved = dispatch(program, func, block, f->functionExpr());
	if (!funcResolved) {
		std::cerr << "Unable to resolve function call:\n";
		f->functionExpr()->print();
		abort();
	}

	const ExprList *args = f->args();
	std::vector <RValue *> exprResults = generateExprList(program, func, block, args);

	RValue *result = program.allocRValue();

	RUNCALL(RUNCALL_PUSH, result);
	for (auto i = exprResults.crbegin(); i != exprResults.crend(); ++i)
		RUNCALL(RUNCALL_PUSH, *i);
	RUNCALL(RUNCALL_PUSH, toVoidPtr(args->exprs().size()));
	RUNCALL(RUNCALL_PUSH, funcResolved);
	RUNCALL(RUNCALL_FUNCTION_CALL, nullptr);

	return result;
}

template <>
RValue * generate<Node::Type::TableCtor>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const TableCtor *tv = static_cast<const TableCtor *>(src);

	RValue *result = program.allocRValue();
	RUNCALL(RUNCALL_PUSH, result);

	const auto &fields = tv->fields();
	int fieldCounter = 0;
	for (const auto &field : fields) {
		if (field->fieldType() == Field::Type::NoIndex) {
			RUNCALL(RUNCALL_PUSH, dispatch(program, func, block, field->valueExpr()));
			RUNCALL(RUNCALL_PUSH, program.allocRValue(RValue{++fieldCounter}));
		}
	}

	for (const auto &field : fields) {
		if (field->fieldType() != Field::Type::NoIndex) {
			RValue *index;
			switch (field->fieldType()) {
				case Field::Type::Brackets:
					index = dispatch(program, func, block, field->keyExpr());
					break;
				case Field::Type::Literal:
					index = program.allocRValue(RValue{field->fieldName()});
					break;
				default:
					break;
			}
			RUNCALL(RUNCALL_PUSH, dispatch(program, func, block, field->valueExpr()));
			RUNCALL(RUNCALL_PUSH, index);
		}
	}

	RUNCALL(RUNCALL_PUSH, toVoidPtr(fields.size()));
	RUNCALL(RUNCALL_TABLE_CTOR, nullptr);

	return result;
}

template <>
RValue * generate<Node::Type::Chunk>(Program &program, gcc_jit_function *func, gcc_jit_block *, const Node *src)
{
	const Chunk *c = static_cast<const Chunk *>(src);
	gcc_jit_block *block = gcc_jit_function_new_block(func, nullptr);
	RUNCALL(RUNCALL_SCOPE_PUSH, nullptr);

	for (const auto &n : c->children())
		dispatch(program, func, block, n.get());

	gcc_jit_block_end_with_void_return(block, nullptr);
	return nullptr;
}

template <>
RValue * generate<Node::Type::Value>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const Value *v = static_cast<const Value *>(src);

	switch (v->valueType()) {
		case ValueType::Nil:
			return program.allocRValue(RValue::Nil());
		case ValueType::Boolean:
			return program.allocRValue(RValue{static_cast<const BooleanValue *>(v)->value()});
		case ValueType::Integer:
			return program.allocRValue(RValue{static_cast<const IntValue *>(v)->value()});
		case ValueType::Real:
			return program.allocRValue(RValue{static_cast<const RealValue *>(v)->value()});
		case ValueType::String:
			return program.allocRValue(RValue{static_cast<const StringValue *>(v)->value()});
		default:
			break;
	}

	std::cerr << "generate<Node::Type::Value>() not implemented for value type: " << prettyPrint(v->valueType()) << '\n';
	abort();
	return nullptr;
}

template <>
RValue * generate<Node::Type::LValue>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const LValue *lval = static_cast<const LValue *>(src);
	RValue *result = program.allocRValue();
	RUNCALL(RUNCALL_PUSH, result);

	switch (lval->lvalueType()) {
		case LValue::Type::Bracket:
		case LValue::Type::Dot: {
			if (lval->lvalueType() == LValue::Type::Dot)
				RUNCALL(RUNCALL_PUSH, program.allocRValue(lval->name()));
			else
				RUNCALL(RUNCALL_PUSH, dispatch(program, func, block, lval->keyExpr()));
			RUNCALL(RUNCALL_PUSH, dispatch(program, func, block, lval->tableExpr()));
			RUNCALL(RUNCALL_TABLE_ACCESS, nullptr);
			break;
		}

		case LValue::Type::Name: {
			RUNCALL(RUNCALL_PUSH, program.duplicateString(lval->name()));
			RUNCALL(RUNCALL_RESOLVE_NAME, nullptr);
			break;
		}
	}

	result->setType(RValue::Type::LValue);
	return result;
}

template <>
RValue * generate<Node::Type::Assignment>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const Assignment *c = static_cast<const Assignment *>(src);
	const ExprList *exprList = c->exprList();
	std::vector <RValue *> exprResults = generateExprList(program, func, block, exprList);

	size_t i = 0;
	const VarList *varList = c->varList();
	for (const auto &lval : varList->vars()) {
		RValue *dst = dispatch(program, func, block, lval.get());
		if (i < exprResults.size())
			RUNCALL(RUNCALL_PUSH, exprResults[i]);
		else
			RUNCALL(RUNCALL_PUSH, program.allocRValue(RValue::Nil()));
		RUNCALL(RUNCALL_PUSH, dst);
		RUNCALL(RUNCALL_ASSIGN, nullptr);

		++i;
	}

	return nullptr;
}

template <typename T>
RValue * generateImmediate(Program &program, const RValue *operand, UnOp::Type op)
{
	return program.allocRValue(RValue::executeUnOp<T>(*operand, op));
}

RValue * generateImmediate(Program &program, RValue *operand, UnOp::Type op)
{
	switch (operand->valueType()) {
		case ValueType::Boolean:
			return generateImmediate<bool>(program, operand, op);
		case ValueType::Integer:
			return generateImmediate<int>(program, operand, op);
		case ValueType::Real:
			return generateImmediate<double>(program, operand, op);
		default:
			break;
	}

	assert(false);
	return nullptr;
}

template <>
RValue * generate<Node::Type::UnOp>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const UnOp *uo = static_cast<const UnOp *>(src);
	RValue *operand = dispatch(program, func, block, uo->operand());
	checkType(operand, uo->operand());

	if (operand->type() == RValue::Type::Immediate)
		return generateImmediate(program, operand, uo->unOpType());

	RValue *result = program.allocRValue();

	RUNCALL(RUNCALL_PUSH, operand);
	RUNCALL(RUNCALL_PUSH, result);
	RUNCALL(RUNCALL_UNOP, toVoidPtr(uo->unOpType()));

	result->setType(RValue::Type::Temporary);
	return result;
}

template <typename T>
RValue * generateImmediate(Program &program, const RValue *opLeft, const RValue *opRight, BinOp::Type op)
{
	return program.allocRValue(RValue::executeBinOp<T>(*opLeft, *opRight, op));
}

RValue * generateImmediate(Program &program, RValue *opLeft, RValue *opRight, BinOp::Type op)
{
	matchTypes(*opLeft, *opRight);
	if (!BinOp::isApplicable(op, opLeft->valueType())) {
		std::cerr << "Binary operation " << BinOp::toString(op) << " not possible for type " << prettyPrint(opLeft->valueType()) << '\n';
		abort();
	}

	switch (opLeft->valueType()) {
		case ValueType::Boolean:
			return generateImmediate<bool>(program, opLeft, opRight, op);
		case ValueType::Integer:
			return generateImmediate<int>(program, opLeft, opRight, op);
		case ValueType::Real:
			return generateImmediate<double>(program, opLeft, opRight, op);
		case ValueType::String:
			return generateImmediate<std::string>(program, opLeft, opRight, op);
		default:
			break;
	}

	assert(false);
	return nullptr;
}

template <>
RValue * generate<Node::Type::BinOp>(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
{
	const BinOp *bo = static_cast<const BinOp *>(src);

	RValue *left = dispatch(program, func, block, bo->left());
	checkType(left, bo->left());

	RValue *right = dispatch(program, func, block, bo->right());
	checkType(left, bo->right());

	if (left->type() == RValue::Type::Immediate && right->type() == RValue::Type::Immediate)
		return generateImmediate(program, left, right, bo->binOpType());

	RValue *result = program.allocRValue();

	RUNCALL(RUNCALL_PUSH, right);
	RUNCALL(RUNCALL_PUSH, left);
	RUNCALL(RUNCALL_PUSH, result);
	RUNCALL(RUNCALL_BINOP, toVoidPtr(bo->binOpType()));

	result->setType(RValue::Type::Temporary);
	return result;
}

RValue * dispatch(Program &program, gcc_jit_function *func, gcc_jit_block *block, const Node *src)
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
	return nullptr;
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
