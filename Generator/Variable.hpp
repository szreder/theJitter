#pragma once

#include <iostream>

#include "Generator/Value.hpp"

class Variable {
public:
	const char *& name() { return m_name; }
	const char * name() const { return m_name; }

	ValueType & type() { return m_value.first; }
	const ValueType & type() const { return m_value.first; }

	Value & value() { return m_value; }
	const Value & value() const { return m_value; }

	Value * asLValue() { return &m_value; }

private:
	const char *m_name;
	Value m_value;
};

std::ostream & operator << (std::ostream &os, const Variable &v);
