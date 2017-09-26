#include <array>
#include <string>

#include "Generator/AST.hpp"
#include "Generator/RValue.hpp"
#include "Util/EnumHelpers.hpp"

const std::string & prettyPrint(Node::Type t)
{
	typedef std::array <std::string, toUnderlying(Node::Type::_last)> StringArray;
	static const StringArray Strings = []{
		StringArray result;

		result[toUnderlying(Node::Type::Chunk)] = "chunk";
		result[toUnderlying(Node::Type::ExprList)] = "expression_list";
		result[toUnderlying(Node::Type::VarList)] = "variable_list";
		result[toUnderlying(Node::Type::Variable)] = "variable";
		result[toUnderlying(Node::Type::FunctionCall)] = "function_call";
		result[toUnderlying(Node::Type::Assignment)] = "assignment";
		result[toUnderlying(Node::Type::Value)] = "value";
		result[toUnderlying(Node::Type::Field)] = "field";
		result[toUnderlying(Node::Type::BinOp)] = "binary_op";
		result[toUnderlying(Node::Type::UnOp)] = "unary_op";

		return result;
	}();

	return Strings[toUnderlying(t)];
}

const std::string & prettyPrint(RValue::Type t)
{
	typedef std::array <std::string, toUnderlying(RValue::Type::_last)> StringArray;
	static const StringArray Strings = []{
		StringArray result;

		result[toUnderlying(RValue::Type::FunctionCall)] = "function_call";
		result[toUnderlying(RValue::Type::Immediate)] = "immediate";
		result[toUnderlying(RValue::Type::Variable)] = "variable";

		return result;
	}();

	return Strings[toUnderlying(t)];
}

const std::string & prettyPrint(ValueType vt)
{
	typedef std::array <std::string, toUnderlying(ValueType::_last)> StringArray;
	static const StringArray Strings = []{
		StringArray result;

		result[toUnderlying(ValueType::Invalid)] = "invalid";
		result[toUnderlying(ValueType::Unknown)] = "unknown";
		result[toUnderlying(ValueType::Boolean)] = "boolean";
		result[toUnderlying(ValueType::Nil)] = "nil";
		result[toUnderlying(ValueType::Integer)] = "integer";
		result[toUnderlying(ValueType::Real)] = "real";

		return result;
	}();

	return Strings[toUnderlying(vt)];
}
