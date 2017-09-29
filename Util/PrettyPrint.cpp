#include <array>
#include <string>

#include "Generator/AST.hpp"
#include "Generator/RValue.hpp"
#include "Util/EnumHelpers.hpp"

const std::string & prettyPrint(Lua::Node::Type t)
{
	typedef std::array <std::string, toUnderlying(Lua::Node::Type::_last)> StringArray;
	static const StringArray Strings = []{
		StringArray result;

		result[toUnderlying(Lua::Node::Type::Chunk)] = "chunk";
		result[toUnderlying(Lua::Node::Type::ExprList)] = "expression_list";
		result[toUnderlying(Lua::Node::Type::VarList)] = "variable_list";
		result[toUnderlying(Lua::Node::Type::LValue)] = "l-value";
		result[toUnderlying(Lua::Node::Type::FunctionCall)] = "function_call";
		result[toUnderlying(Lua::Node::Type::Assignment)] = "assignment";
		result[toUnderlying(Lua::Node::Type::Value)] = "value";
		result[toUnderlying(Lua::Node::Type::Field)] = "field";
		result[toUnderlying(Lua::Node::Type::TableCtor)] = "table_constructor";
		result[toUnderlying(Lua::Node::Type::BinOp)] = "binary_op";
		result[toUnderlying(Lua::Node::Type::UnOp)] = "unary_op";

		return result;
	}();

	return Strings[toUnderlying(t)];
}

const std::string & prettyPrint(RValue::Type t)
{
	typedef std::array <std::string, toUnderlying(RValue::Type::_last)> StringArray;
	static const StringArray Strings = []{
		StringArray result;

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
		result[toUnderlying(ValueType::Function)] = "function";
		result[toUnderlying(ValueType::Table)] = "table";

		return result;
	}();

	return Strings[toUnderlying(vt)];
}
