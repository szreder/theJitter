#include <array>
#include <string>

#include "Generator/ValueType.hpp"
#include "Util/EnumHelpers.hpp"

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
