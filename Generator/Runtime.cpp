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

namespace {

std::vector <Scope> scopeStack;
std::vector <void *> dataStack;

void __setVariable(const std::string *varName, const RValue *value);
void __setVariable(const std::string *varName, RValue &&value);

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

std::pair <Scope *, const Variable *> __findScope(const std::string *varName)
{
	for (size_t i = scopeStack.size(); i > 0; --i) {
		Scope *scope = &scopeStack[i - 1];
		auto var = scope->getVariable(varName);
		if (var)
			return std::make_pair(scope, var);
	}

	return {nullptr, nullptr};
}

RValue resolveRValue(const RValue *src)
{
	if (src->type() == RValue::Type::Immediate)
		return *src;

	switch (src->type()) {
		case RValue::Type::Variable: {
			const auto &varName = src->value<std::string>();
			auto [_, var] = __findScope(&varName);
			if (!var) {
				std::cerr << "Cannot resolve variable: " << varName << '\n';
				abort();
			}
			return RValue{var};
		} default:
			std::cerr << "Cannot resolve RValue of type = " << toUnderlying(src->type()) << '\n';
			abort();
	}

	return RValue{};
}

void executeBinOp(Lua::BinOp::Type op)
{
	std::cout << "Execute BinOp " << Lua::BinOp::toString(op) << '\n';
	const RValue *left = popData<const RValue *>();
	const RValue *right = popData<const RValue *>();
	const std::string *varName = popData<const std::string *>();

	RValue opLeft = resolveRValue(left);
	RValue opRight = resolveRValue(right);
	matchTypes(opLeft, opRight);

	RValue result;
	switch (opLeft.valueType()) {
		case ValueType::Boolean:
			result = RValue::executeBinOp<bool>(opLeft, opRight, op);
			break;
		case ValueType::Integer:
			result = RValue::executeBinOp<int>(opLeft, opRight, op);
			break;
		case ValueType::Real:
			result = RValue::executeBinOp<double>(opLeft, opRight, op);
			break;
		case ValueType::String:
			result = RValue::executeBinOp<std::string>(opLeft, opRight, op);
			break;
		default:
			std::cerr << "Binary operation " << Lua::BinOp::toString(op) << " not possible for type " << prettyPrint(opLeft.valueType()) << '\n';
			abort();
	}

	__setVariable(varName, &result);
}

void executeUnOp(Lua::UnOp::Type op)
{
	const RValue *operand = popData<const RValue *>();
	const std::string *varName = popData<const std::string *>();

	RValue result = resolveRValue(operand);
	switch (result.valueType()) {
		case ValueType::Boolean:
			result = RValue::executeUnOp<bool>(result, op);
			break;
		case ValueType::Integer:
			result = RValue::executeUnOp<int>(result, op);
			break;
		case ValueType::Real:
			result = RValue::executeUnOp<double>(result, op);
			break;
		default:
			std::cerr << "Unary operation " << Lua::UnOp::toString(op) << " not possible for type " << prettyPrint(result.valueType()) << '\n';
			break;
	}

	__setVariable(varName, &result);
}

void executeFunctionCall()
{
	const RValue *rval_fn = popData<const RValue *>();
	RValue resolvedSymbol = resolveRValue(rval_fn);

	size_t argCnt = popData<size_t>();
	__arg_vec args(argCnt);
	for (size_t i = 0; i != argCnt; ++i)
		args[i] = resolveRValue(popData<const RValue *>());

	if (resolvedSymbol.valueType() != ValueType::Function) {
		std::cerr << "Attempted to call " << resolvedSymbol << '\n';
		abort();
	}

	auto func = resolvedSymbol.value<fn_ptr>();
	func(&args);
}

void constructTable()
{
	size_t fieldCnt = popData<size_t>();

	std::shared_ptr <Table> table = std::make_shared<Table>();

	while (fieldCnt != 0) {
		--fieldCnt;
		RValue key = resolveRValue(popData<const RValue *>());
		RValue value = resolveRValue(popData<const RValue *>());
		table->setValue(key, value);
	}

	const std::string *varName = popData<const std::string *>();

	RValue result{table};
	__setVariable(varName, &result);
}

void unsetVariable()
{
	const std::string *varName = popData<const std::string *>();
	auto [scope, var] = __findScope(varName);

	if (scope)
		scope->removeVariable(var);
}

void setVariable()
{
	const RValue *value = popData<const RValue *>();
	const std::string *varName = popData<const std::string *>();
	__setVariable(varName, value);
}

template <typename T>
void __doSetVariable(const std::string *varName, T &&value)
{
	auto [scope, var] = __findScope(varName);
	if (scope)
		scope->setVariable(varName, std::forward<T>(value));
	else
		scopeStack.back().setVariable(varName, std::forward<T>(value));
}

void __setVariable(const std::string *varName, const RValue *value)
{
	std::cout << "setVariable " << *varName << '\n';

	switch (value->type()) {
		case RValue::Type::Immediate: {
			__doSetVariable(varName, value);
			break;
		}

		case RValue::Type::Variable: {
			const std::string &searchedName = value->value<std::string>();
			auto [scope, var] = __findScope(&searchedName);

			if (!scope) {
				std::cerr << "Variable " << searchedName << " not in scope\n";
				abort();
			}

			__doSetVariable(varName, var);
			break;
		}

		default:
			std::cerr << "setVariable(varName = " << *varName << "), unknown RValue type = " << toUnderlying(value->type()) << '\n';
			abort();
	}
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
	std::cout << "==== Runcall " << call << " with arg = " << arg << '\n';
	switch (call) {
		case RUNCALL_SCOPE_PUSH:
			std::cout << "Scope push\n";
			scopeStack.push_back(Scope{});
			break;
		case RUNCALL_SCOPE_POP:
			std::cout << "Scope pop\n";
			scopeStack.pop_back();
			break;
		case RUNCALL_PUSH:
			dataStack.push_back(arg);
			break;
		case RUNCALL_VARIABLE_UNSET:
			std::cout << "Unset variable\n";
			unsetVariable();
			break;
		case RUNCALL_VARIABLE_SET:
			std::cout << "Set variable\n";
			setVariable();
			break;
		case RUNCALL_BINOP:
			executeBinOp(fromVoidPtr<Lua::BinOp::Type>(arg));
			break;
		case RUNCALL_UNOP:
			executeUnOp(fromVoidPtr<Lua::UnOp::Type>(arg));
			break;
		case RUNCALL_FUNCTION_CALL:
			executeFunctionCall();
			break;
		case RUNCALL_TABLE_CTOR:
			constructTable();
			break;
		default:
			std::cout << "Runcall " << call << " not supported\n";
	}
}
