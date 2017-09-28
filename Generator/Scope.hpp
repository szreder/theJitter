#pragma once

#include <string_view>
#include <unordered_set>

#include "Generator/RValue.hpp"
#include "Generator/ValueType.hpp"
#include "Generator/Variable.hpp"
#include "Util/PrettyPrint.hpp"

class Scope {
public:
	const Variable * getVariable(const std::string *varName)
	{
		Variable tmp;
		tmp.name = varName->data();
		auto var = m_vars.find(tmp);
		if (var != m_vars.end())
			return &(*var);
		return nullptr;
	}

	void setVariable(const std::string *varName, const RValue *value)
	{
		Variable tmp;
		tmp.name = varName->data();
		tmp.type = value->valueType();
		tmp.value = value->value();

		m_vars.erase(tmp);
		m_vars.insert(tmp);
	}

	void setVariable(const std::string *varName, const Variable *value)
	{
		std::cerr << "Scope, set variable = " << *varName << " to: " << *value << '\n';
		Variable tmp;
		tmp.name = varName->data();
		tmp.type = value->type;
		tmp.value = value->value;
		m_vars.erase(tmp);
		m_vars.insert(tmp);
	}

	bool removeVariable(const std::string *varName)
	{
		Variable tmp;
		tmp.name = varName->data();
		return m_vars.erase(tmp) != 0;
	}

	bool removeVariable(const Variable *var)
	{
		return m_vars.erase(*var);
	}

private:
	std::unordered_set <Variable, VariableHasher, VariableComparer> m_vars;
};
