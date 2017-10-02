#pragma once

#include "Generator/ValueType.hpp"
#include "Generator/ValueVariant.hpp"

typedef std::pair <ValueType, ValueVariant> Value;

std::ostream & operator << (std::ostream &os, const Value &v);
