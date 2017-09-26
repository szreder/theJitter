#pragma once

#include <string>

#include "Generator/AST.hpp"
#include "Generator/ValueType.hpp"

const std::string & prettyPrint(Node::Type t);
const std::string & prettyPrint(ValueType vt);
