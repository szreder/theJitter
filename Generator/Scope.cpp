#include <cstring>
#include <string_view>

#include "Generator/Scope.hpp"
#include "Generator/Variable.hpp"
#include "Util/PrettyPrint.hpp"

Variable * Scope::getVariable(const std::string *varName)
{
	auto var = m_vars.find(*varName);
	if (var != m_vars.end())
		return &var->second;
	return nullptr;
}

Variable * Scope::setVariable(const std::string *varName, const RValue *value)
{
	Variable tmp;
	tmp.name() = varName->data();
	tmp.type() = value->valueType();
	tmp.value() = value->value();

	return &m_vars.insert_or_assign(*varName, tmp).first->second;
}

Variable * Scope::setVariable(const std::string *varName, const Variable *value)
{
	Variable tmp;
	tmp.name() = varName->data();
	tmp.type() = value->type();
	tmp.value() = value->value();

	return &m_vars.insert_or_assign(*varName, tmp).first->second;
}

bool Scope::removeVariable(const std::string *varName)
{
	return m_vars.erase(*varName) != 0;
}


bool Scope::removeVariable(const Variable *var)
{
	return m_vars.erase(std::string{var->name()});
}
