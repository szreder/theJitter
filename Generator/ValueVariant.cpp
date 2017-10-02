#include <iostream>

#include "Generator/ValueVariant.hpp"

std::ostream & operator << (std::ostream &os, const ValueVariant &v)
{
	std::visit([&os](auto &&arg){
		os << arg;
	}, v);

	return os;
}
