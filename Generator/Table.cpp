#include "Generator/Table.hpp"

Value Table::value(const RValue &key) const
{
	checkKey(key);
	auto iter = m_data.find(key.value());
	if (iter == m_data.end())
		return {};
}

void Table::setValue(const RValue &key, const RValue &value)
{
	checkKey(key);
	checkValue(value);

	if (value.isNil())
		m_data.erase(key.value());
	else
		m_data.insert_or_assign(key.value(), Value{value.valueType(), value.value()});
}

void Table::checkKey(const RValue &key) const
{
	assert(key.type() == RValue::Type::Immediate);
	if (key.isNil()) {
		std::cerr << "Attempting to index table with Nil value\n";
		abort();
	}
}

void Table::checkValue(const RValue &value) const
{
	assert(value.type() == RValue::Type::Immediate);
}

std::ostream & operator << (std::ostream &os, const Table &t)
{
	return os;
}
