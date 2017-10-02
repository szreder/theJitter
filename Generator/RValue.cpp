#include <iostream>

#include "Generator/RValue.hpp"
#include "Util/PrettyPrint.hpp"

void matchTypes(RValue &leftRValue, RValue &rightRValue)
{
	if (leftRValue.valueType() == ValueType::Integer && rightRValue.valueType() == ValueType::Real) {
		leftRValue.setValueType(ValueType::Real);
		leftRValue.setValue<double>(leftRValue.value<int>());
	}

	if (leftRValue.valueType() == ValueType::Real && rightRValue.valueType() == ValueType::Integer) {
		rightRValue.setValueType(ValueType::Real);
		rightRValue.setValue<double>(rightRValue.value<int>());
	}

	if (leftRValue.valueType() != rightRValue.valueType()) {
		std::cerr << "Type mismatch: " << prettyPrint(leftRValue.valueType()) << " vs " << prettyPrint(rightRValue.valueType()) << '\n';
		abort();
	}
}

std::ostream & operator << (std::ostream &os, const RValue &rv)
{
	os << "RValue(" << rv.value() << ')';
	return os;
}
