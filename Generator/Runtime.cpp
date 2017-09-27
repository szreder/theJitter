#include <cassert>
#include <iostream>
#include <vector>

#include "Generator/AST.hpp"
#include "Generator/Builtins.hpp"
#include "Generator/Program.hpp"
#include "Generator/RValue.hpp"
#include "Generator/Runtime.hpp"
#include "Generator/Scope.hpp"
#include "Util/Casts.hpp"

namespace {

std::vector <Scope> scopeStack;
std::vector <void *> dataStack;

void __setVariable(const std::string *varName, const RValue *value);

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

const Variable * findVariable(const std::string *varName)
{
	for (auto scope = scopeStack.rbegin(); scope != scopeStack.rend(); ++scope) {
		auto result = scope->getVariable(varName);
		if (result != nullptr)
			return result;
	}
	return nullptr;
}

const Variable * findVariable(const std::string &varName)
{
	return findVariable(&varName);
}

RValue resolveRValue(const RValue *src)
{
	if (src->type() == RValue::Type::Immediate)
		return *src;

	switch (src->type()) {
		case RValue::Type::Variable: {
			const auto &varName = src->value<std::string>();
			auto var = findVariable(varName);
			if (!var) {
				std::cerr << "Cannot resolve variable: " << varName << '\n';
				abort();
			}
			return RValue::fromVariable(var);
		} default:
			std::cerr << "Cannot resolve RValue of type = " << toUnderlying(src->type()) << '\n';
			abort();
	}

	return RValue{};
}

void executeBinOp(BinOp::Type op)
{
	std::cout << "Execute BinOp " << BinOp::toString(op) << '\n';
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
			std::cerr << "Binary operation " << BinOp::toString(op) << " not possible for type " << prettyPrint(opLeft.valueType()) << '\n';
			abort();
	}

	__setVariable(varName, &result);
}

void executeUnOp(UnOp::Type op)
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
			std::cerr << "Unary operation " << UnOp::toString(op) << " not possible for type " << prettyPrint(result.valueType()) << '\n';
			break;
	}

	__setVariable(varName, &result);
}

void callFunction(fn_ptr func, const std::vector <const RValue *> args)
{
	//TODO argument passing (after Table implementation)
	func(nullptr);
}

void executeFunctionCall()
{
	const RValue *func = popData<const RValue *>();
	RValue resolvedSymbol = resolveRValue(func);

	size_t argCnt = popData<size_t>();
	std::vector <const RValue *> args(argCnt);
	for (size_t i = 0; i != argCnt; ++i)
		args[i] = popData<const RValue *>();


	if (resolvedSymbol.valueType() != ValueType::Function) {
		std::cerr << "Attempted to call " << resolvedSymbol << '\n';
		abort();
	}
	callFunction(resolvedSymbol.value<fn_ptr>(), args);
}

void unsetVariable()
{
	const std::string *varName = popData<const std::string *>();
	auto var = findVariable(varName);

	if (!var)
		return;

	for (auto scope = scopeStack.rbegin(); scope != scopeStack.rend(); ++scope) {
		if (scope->removeVariable(varName))
			return;
	}
}

void setVariable()
{
	const RValue *value = popData<const RValue *>();
	const std::string *varName = popData<const std::string *>();
	__setVariable(varName, value);
}

void __setVariable(const std::string *varName, const RValue *value)
{
	std::cout << "setVariable " << *varName << '\n';

	auto doSetVar = [](const std::string *name, auto val, std::vector <Scope> &scopeStack) {
		for (auto scope = scopeStack.rbegin(); scope != scopeStack.rend(); ++scope) {
			auto var = scope->getVariable(name);
			if (var) {
				scope->setVariable(name, val);
				return;
			}
		}

		scopeStack.back().setVariable(name, val);
	};

	switch (value->type()) {
		case RValue::Type::Immediate: {
			doSetVar(varName, value, scopeStack);
			break;
		}

		case RValue::Type::Variable: {
			const Variable *otherVar = nullptr;
			const std::string &searchedName = value->value<std::string>();

			std::cerr << "setVariable(varName = " << *varName << ") from varName = " << searchedName << '\n';

			auto scope = scopeStack.rbegin();
			while (scope != scopeStack.rend() && otherVar == nullptr) {
				otherVar = scope->getVariable(&searchedName);
				++scope;
			}

			if (otherVar == nullptr) {
				std::cerr << "Variable " << searchedName << " not in scope\n";
				abort();
			}

			doSetVar(varName, otherVar, scopeStack);
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
	RValue tmp{&ping};
	std::string *name = program.duplicateString("ping");
	s.setVariable(name, &tmp);
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
			executeBinOp(fromVoidPtr<BinOp::Type>(arg));
			break;
		case RUNCALL_UNOP:
			executeUnOp(fromVoidPtr<UnOp::Type>(arg));
			break;
		case RUNCALL_FUNCTION_CALL:
			executeFunctionCall();
			break;
		default:
			std::cout << "Runcall " << call << " not supported\n";
	}
}
