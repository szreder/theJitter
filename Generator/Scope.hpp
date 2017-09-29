#pragma once

#include <string_view>
#include <unordered_set>

#include "Generator/RValue.hpp"
#include "Generator/ValueType.hpp"
#include "Generator/Variable.hpp"
#include "Util/PrettyPrint.hpp"

class Scope {
public:
	const Variable * getVariable(const std::string *varName);
	void setVariable(const std::string *varName, const RValue *value);
	void setVariable(const std::string *varName, const Variable *value);
	bool removeVariable(const std::string *varName);
	bool removeVariable(const Variable *var);

private:
	std::unordered_set <Variable, VariableHasher, VariableComparer> m_vars;
};
