#pragma once

#include <libgccjit.h>
#include <variant>

#include "Generator/Program.hpp"
#include "Generator/ValueType.hpp"
#include "Generator/Variable.hpp"

class RValue {
public:
	enum class Type {
		FunctionCall,
		Immediate,
		Variable,
	};

	static RValue asVariable(const std::string &varName)
	{
		RValue result;
		result.m_type = Type::Variable;
		result.m_valueType = ValueType::Unknown;
		result.m_value = varName;
		return result;
	}

	static RValue fromVariable(const Variable *var)
	{
		switch (var->type) {
			case ValueType::Boolean:
				return RValue{var->b};
			case ValueType::Integer:
				return RValue{var->i};
			case ValueType::Real:
				return RValue{var->r};
			case ValueType::String:
				return RValue{var->s};
		}

		return RValue{};
	}

	RValue() : m_type{Type::Immediate}, m_valueType{ValueType::Invalid} {}
	RValue(bool v) : m_type{Type::Immediate}, m_valueType{ValueType::Boolean}, m_value{v} {}
	RValue(int v) : m_type{Type::Immediate}, m_valueType{ValueType::Integer}, m_value{v} {}
	RValue(double v) : m_type{Type::Immediate}, m_valueType{ValueType::Real}, m_value{v} {}
	RValue(const std::string &v) : m_type{Type::Immediate}, m_valueType{ValueType::String}, m_value{v} {}
	RValue(std::string &&v) : m_type{Type::Immediate}, m_valueType{ValueType::String}, m_value{std::move(v)} {}

	~RValue() = default;

	static const RValue & Nil()
	{
		static const RValue NilValue = [] {
			RValue result;
			result.m_type = Type::Immediate;
			result.m_valueType = ValueType::Nil;
			return result;
		}();
		return NilValue;
	}

	Type type() const { return m_type; }

	ValueType valueType() const { return m_valueType; }
	void setValueType(ValueType vt) { m_valueType = vt; }

	template <typename T>
	const T & value() const { return std::get<T>(m_value); }

	template <typename T>
	void setValue(const T &v) { m_value = v; }

private:
	Type m_type;
	ValueType m_valueType;
	std::variant <bool, int, double, std::string> m_value;
};

inline void matchTypes(RValue &leftRValue, RValue &rightRValue)
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
