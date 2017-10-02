#include "Generator/Table.hpp"
#include "Generator/Variable.hpp"
#include "Util/PrettyPrint.hpp"

std::ostream & operator << (std::ostream &os, const Variable &v)
{
	os << "Variable: {name = " << v.name() << ", " << v.value() << '}';

	return os;
}
