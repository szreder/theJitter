#include <cassert>
#include <iostream>
#include <vector>

#include "Generator/AST.hpp"
#include "Generator/Builtins.hpp"
#include "Generator/Program.hpp"
#include "Generator/RValue.hpp"
#include "Generator/Runtime.hpp"
#include "Generator/Scope.hpp"
#include "Generator/Table.hpp"
#include "Util/Casts.hpp"
#include "Util/PrettyPrint.hpp"

namespace {

std::vector <Scope> scopeStack;
std::vector <void *> dataStack;

template <typename T>
T popData()
{
	assert(!dataStack.empty());
	void *p = dataStack.back();
	dataStack.pop_back();
	if constexpr(std::is_integral<T>::value)
		return fromVoidPtr<T>(p);
	else
		return static_cast<T>(p);
}

std::pair <Scope *, Variable *> __findScope(const std::string *varName)
{
	for (size_t i = scopeStack.size(); i > 0; --i) {
		Scope *scope = &scopeStack[i - 1];
		auto var = scope->getVariable(varName);
		if (var)
			return std::make_pair(scope, var);
	}

	return {nullptr, nullptr};
}

void initVariable()
{
	const std::string *varName = popData<std::string *>();
	scopeStack.back().setVariable(varName, &RValue::Nil());
}

void executeAssign()
{
	RValue *dst = popData<RValue *>();
	const RValue *src = popData<RValue *>();

#ifndef NDEBUG
	std::cout << "Assign value: " << *src << '\n';
#endif

	assert(dst->type() == RValue::Type::LValue);
	*dst->lvalue() = src->value();
}

void executeUnOp(Lua::UnOp::Type op)
{
	RValue *dst = popData<RValue *>();
	const RValue *src = popData<RValue *>();
	dst->setValue(src->value());

	switch (dst->valueType()) {
		case ValueType::Boolean:
			dst->executeUnOp<bool>(op);
			break;
		case ValueType::Integer:
			dst->executeUnOp<int>(op);
			break;
		case ValueType::Real:
			dst->executeUnOp<double>(op);
			break;
		default:
			std::cerr << "Unary operation " << Lua::UnOp::toString(op) << " not possible for type " << prettyPrint(dst->valueType()) << '\n';
			break;
	}
}

void executeBinOp(Lua::BinOp::Type op)
{
	RValue *dst = popData<RValue *>();
	RValue *left = popData<RValue *>();
	RValue *right = popData<RValue *>();

	matchTypes(*left, *right);
	dst->setValue(left->value());

	switch (left->valueType()) {
		case ValueType::Boolean:
			dst->executeBinOp<bool>(*right, op);
			break;
		case ValueType::Integer:
			dst->executeBinOp<int>(*right, op);
			break;
		case ValueType::Real:
			dst->executeBinOp<double>(*right, op);
			break;
		case ValueType::String:
			dst->executeBinOp<std::string>(*right, op);
			break;
		default:
			std::cerr << "Binary operation " << Lua::BinOp::toString(op) << " not possible for type " << prettyPrint(left->valueType()) << '\n';
			abort();
	}
}

void executeFunctionCall()
{
	const RValue *rval_fn = popData<RValue *>();

	size_t argCnt = popData<size_t>();
	__arg_vec args(argCnt);
	for (size_t i = 0; i != argCnt; ++i)
		args[i] = popData<RValue *>();

	if (rval_fn->valueType() != ValueType::Function) {
		std::cerr << "Attempted to call value of type " << prettyPrint(rval_fn->valueType()) << '\n';
		abort();
	}

	RValue *result = popData<RValue *>();
	auto func = rval_fn->value<fn_ptr>();
	func(&args, result);
}

void resolveName()
{
	const std::string *varName = popData<const std::string *>();
	RValue *dst = popData<RValue *>();

	Variable *var = __findScope(varName).second;
	if (var == nullptr)
		var = scopeStack.back().setVariable(varName, &RValue::Nil());

	dst->setLValue(var->asLValue());
}

void constructTable()
{
	size_t fieldCnt = popData<size_t>();

	std::shared_ptr <Table> table = std::make_shared<Table>();

	while (fieldCnt != 0) {
		--fieldCnt;
		RValue *key = popData<RValue *>();
		RValue *value = popData<RValue *>();
		table->setValue(*key, *value);
	}

	RValue *result = popData<RValue *>();
	result->setValue(table);
	result->setValueType(ValueType::Table);
}

void accessTable()
{
	const RValue *tableValue = popData<RValue *>();
	const RValue *keyValue = popData<RValue *>();

	if (tableValue->valueType() != ValueType::Table) {
		std::cerr << "Attempted to index non-table type\n";
		abort();
	}

	RValue *result = popData<RValue *>();
	result->setLValue(tableValue->value<std::shared_ptr <Table> >()->value(*keyValue));
}

} //namespace

void initRuntime(Program &program)
{
	Scope s;

#define export(funcName) \
	{ \
		std::string *name = program.duplicateString(#funcName); \
		RValue tmp{&funcName}; \
		s.setVariable(name, &tmp); \
	} \

	export(__ping);
	export(print);
#undef export

	scopeStack.push_back(s);
}

void runcall(RuncallNum call, void *arg)
{
	switch (call) {
		case RUNCALL_SCOPE_PUSH:
			scopeStack.push_back(Scope{});
			break;
		case RUNCALL_SCOPE_POP:
			scopeStack.pop_back();
			break;
		case RUNCALL_PUSH:
			dataStack.push_back(arg);
			break;
		case RUNCALL_INIT_VARIABLE:
			initVariable();
			break;
		case RUNCALL_RESOLVE_NAME:
			resolveName();
			break;
		case RUNCALL_ASSIGN:
			executeAssign();
			break;
		case RUNCALL_UNOP:
			executeUnOp(fromVoidPtr<Lua::UnOp::Type>(arg));
			break;
		case RUNCALL_BINOP:
			executeBinOp(fromVoidPtr<Lua::BinOp::Type>(arg));
			break;
		case RUNCALL_FUNCTION_CALL:
			executeFunctionCall();
			break;
		case RUNCALL_TABLE_CTOR:
			constructTable();
			break;
		case RUNCALL_TABLE_ACCESS:
			accessTable();
			break;
		default:
			std::cout << "Runcall " << call << " not supported\n";
	}
}
