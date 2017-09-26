#pragma once

#include <string_view>
#include <unordered_set>

#include "Generator/RValue.hpp"
#include "Generator/ValueType.hpp"
#include "Generator/Variable.hpp"

class Scope {
public:
	const Variable * getVar(const char *varName)
	{
		Variable tmp;
		tmp.name = varName;
		auto var = m_vars.find(tmp);
		if (var != m_vars.end())
			return &(*var);
		return nullptr;
	}

	void setVar(const char *varName, const RValue *value)
	{
		Variable tmp;
		tmp.name = varName;
		tmp.type = value->valueType();
		switch (tmp.type) {
			case ValueType::Boolean:
				tmp.b = value->value<bool>();
				break;
			case ValueType::Integer:
				tmp.i = value->value<int>();
				break;
			case ValueType::Real:
				tmp.r = value->value<double>();
				break;
			case ValueType::Nil:
				break;
			default:
				std::cerr << "Unsupported value type: " << toUnderlying(tmp.type) << '\n';
				break;
		}
		m_vars.insert(tmp);
	}

	void setVar(const char *varName, const Variable *value)
	{
		std::cerr << "Scope, set variable = " << varName << " to: " << *value << '\n';
		Variable tmp;
		tmp.name = varName;
		tmp.type = value->type;
		switch (tmp.type) {
			case ValueType::Boolean:
				tmp.b = value->b;
				break;
			case ValueType::Integer:
				tmp.i = value->i;
				break;
			case ValueType::Real:
				tmp.r = value->r;
				break;
			case ValueType::Nil:
				break;
			default:
				std::cerr << "Unsupported value type: " << toUnderlying(tmp.type) << '\n';
				break;
		}
		m_vars.insert(tmp);
	}

	bool removeVar(const char *varName)
	{
		Variable tmp;
		tmp.name = varName;
		return m_vars.erase(tmp) != 0;
	}

	bool removeVar(const Variable *var)
	{
		return m_vars.erase(*var);
	}

private:
	std::unordered_set <Variable, VariableHasher, VariableComparer> m_vars;
};
