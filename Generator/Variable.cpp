#include "Generator/Program.hpp"
#include "Generator/ValueType.hpp"

std::ostream & operator << (std::ostream &os, const Variable &v)
{
	os << "Variable: {name = " << v.name << ", type = " << prettyPrint(v.type) << ", value = ";
	switch (v.type) {
		case ValueType::Boolean:
			os << v.b;
			break;
		case ValueType::Integer:
			os << v.i;
			break;
		case ValueType::Real:
			os << v.r;
			break;
		default:
			os << "unknown";
	}

	return os;
}