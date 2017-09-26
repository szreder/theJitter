#include <cassert>
#include <iostream>
#include <vector>

#include "Generator/AST.hpp"
#include "Generator/Program.hpp"
#include "Generator/RValue.hpp"
#include "Generator/Runtime.hpp"
#include "Generator/Scope.hpp"

namespace {

std::vector <Scope> scopeStack;
std::vector <void *> dataStack;

void __setVariable(const std::string *varName, const RValue *value);

template <typename T>
T popData()
{
	assert(!dataStack.empty());
	T result = static_cast<T>(dataStack.back());
	dataStack.pop_back();
	return result;
}

const Variable * findVariable(const std::string *varName)
{
	for (auto scope = scopeStack.rbegin(); scope != scopeStack.rend(); ++scope) {
		auto result = scope->getVar(varName);
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
	}

	__setVariable(varName, &result);
}

void unsetVariable()
{
	const std::string *varName = popData<const std::string *>();
	auto var = findVariable(varName);
	         //TODO
	if (var)

	for (auto scope = scopeStack.rbegin(); scope != scopeStack.rend(); ++scope) {
		if (scope->removeVar(varName))
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
			auto var = scope->getVar(name);
			if (var) {
				scope->setVar(name, val);
				return;
			}
		}

		scopeStack.back().setVar(name, val);
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
				otherVar = scope->getVar(&searchedName);
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

void runcall(int call, void *arg)
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
		case RUNCALL_PUSH_RVALUE:
			std::cout << "Push " << *static_cast<const RValue *>(arg) << '\n';
			dataStack.push_back(arg);
			break;
		case RUNCALL_VARIABLE_NAME:
			std::cout << "Push variable name: " << *static_cast<const std::string *>(arg) << '\n';
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
		default:
			std::cout << "Runcall " << call << " not supported\n";
	}
}
