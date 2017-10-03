#pragma once

#include <unordered_map>

#include "Generator/RValue.hpp"
#include "Generator/ValueType.hpp"
#include "Generator/Variable.hpp"

class Scope {
public:
	Variable * getVariable(const std::string *varName);
	Variable * setVariable(const std::string *varName, const RValue *value);
	Variable * setVariable(const std::string *varName, const Variable *value);
	bool removeVariable(const std::string *varName);
	bool removeVariable(const Variable *var);

private:
	std::unordered_map <std::string, Variable> m_vars; //TODO change std::string to string_view?
};
