#include "Generator/Table.hpp"
#include "Generator/Variable.hpp"
#include "Util/PrettyPrint.hpp"

std::ostream & operator << (std::ostream &os, const Variable &v)
{
	os << "Variable: {name = " << v.name << ", type = " << prettyPrint(v.type) << ", value = ";
	switch (v.type) {
		case ValueType::Boolean:
			os << std::get<bool>(v.value);
			break;
		case ValueType::Integer:
			os << std::get<int>(v.value);
			break;
		case ValueType::Real:
			os << std::get<double>(v.value);
			break;
		case ValueType::Function:
			os << std::get<fn_ptr>(v.value);
			break;
		case ValueType::Table:
// 			os << *static_cast<Table *>(v.v);
			break;
		default:
			os << "unknown";
	}
	os << '}';

	return os;
}
