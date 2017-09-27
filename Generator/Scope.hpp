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
			case ValueType::Function:
				tmp.f = value->value<fn_ptr>();
				break;
			case ValueType::Nil:
				break;
			default:
				std::cerr << "Unsupported value type: " << prettyPrint(tmp.type) << '\n';
				break;
		}
		m_vars.insert(tmp);
	}

	void setVariable(const std::string *varName, const Variable *value)
	{
		std::cerr << "Scope, set variable = " << *varName << " to: " << *value << '\n';
		Variable tmp;
		tmp.name = varName->data();
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
			case ValueType::Function:
				tmp.f = value->f;
				break;
			case ValueType::Nil:
				break;
			default:
				std::cerr << "Unsupported value type: " << prettyPrint(tmp.type) << '\n';
				break;
		}
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
