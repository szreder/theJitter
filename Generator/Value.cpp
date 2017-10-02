#include <iostream>

#include "Generator/Table.hpp"
#include "Generator/Value.hpp"
#include "Util/PrettyPrint.hpp"

std::ostream & operator << (std::ostream &os, const Value &v)
{
	os << "Value: {type = " << prettyPrint(v.first) << ", value = ";
	switch (v.first) {
		case ValueType::Boolean:
			os << std::boolalpha << std::get<bool>(v.second);
			break;
		case ValueType::Integer:
			os << std::get<int>(v.second);
			break;
		case ValueType::Real:
			os << std::get<double>(v.second);
			break;
		case ValueType::String:
			os << std::get<std::string>(v.second);
			break;
		case ValueType::Function:
			os << std::get<fn_ptr>(v.second);
			break;
		case ValueType::Table:
			os << *std::get<std::shared_ptr <Table> >(v.second);
			break;
		default:
			os << "<unknown>";
	}
	os << '}';

	return os;
}
