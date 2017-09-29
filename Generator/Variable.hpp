#pragma once

#include <cstring>
#include <iostream>
#include <memory>
#include <string_view>
#include <unordered_set>
#include <variant>

#include "Generator/Value.hpp"

class Variable {
public:
	const char *& name() { return m_name; }
	const char * name() const { return m_name; }

	ValueType & type() { return m_value.first; }
	const ValueType & type() const { return m_value.first; }

	ValueVariant & value() { return m_value.second; }
	const ValueVariant & value() const { return m_value.second; }

private:
	const char *m_name;
	Value m_value;
};

std::ostream & operator << (std::ostream &os, const Variable &v);

struct VariableHasher {
	size_t operator()(const Variable &v) const
	{
		return std::hash<std::string_view>{}(v.name());
	}
};

struct VariableComparer {
	bool operator()(const Variable &a, const Variable &b) const
	{
		return strcmp(a.name(), b.name()) == 0;
	}
};
