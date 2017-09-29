#pragma once

#include <string>

#include "Generator/AST.hpp"
#include "Generator/RValue.hpp"

const std::string & prettyPrint(Lua::Node::Type t);
const std::string & prettyPrint(RValue::Type t);
const std::string & prettyPrint(ValueType vt);
