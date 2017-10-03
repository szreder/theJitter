#include "Generator/Table.hpp"
#include "Util/Fold.hpp"

Value * Table::value(const RValue &key)
{
	checkKey(key);

	auto iter = m_data.find(key.value().second);
	if (iter == m_data.end())
		return setValue(key, RValue::Nil());
	return &iter->second;
}

Value * Table::setValue(const RValue &key, const RValue &value)
{
	checkKey(key);

	return &m_data.insert_or_assign(key.value().second, value.value()).first->second;
}

void Table::checkKey(const RValue &key) const
{
	if (key.isNil()) {
		std::cerr << "Attempting to index table with Nil value\n";
		abort();
	}
}

std::ostream & operator << (std::ostream &os, const Table &t)
{
	bool first = true;
	os << '{';
	for (const auto &p : t.m_data) {
		if (!first)
			os << ", ";
		first = false;
		os << "{Key: " << p.first << ", " << p.second << '}';
	}
	os << '}';
	return os;
}
