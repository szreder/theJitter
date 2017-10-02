#pragma once

#include <cassert>
#include <iostream>
#include <map>

#include "Generator/RValue.hpp"
#include "Generator/Value.hpp"

class Table {
	friend std::ostream & operator << (std::ostream &os, const Table &t);
public:
	Value value(const RValue &key) const;
	void setValue(const RValue &key, const RValue &value);

private:
	void checkKey(const RValue &key) const;

	std::map <ValueVariant, Value> m_data;
};
