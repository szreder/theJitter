#pragma once

#include <libgccjit.h>
#include <variant>

#include "Generator/AST.hpp"
#include "Generator/ValueType.hpp"
#include "Generator/Variable.hpp"

class RValue {
public:
	enum class Type {
		Immediate,
		Variable,
		_last,
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
			case ValueType::Function:
				return RValue{var->f};
		}

		return RValue{};
	}

	template <typename T>
	static RValue executeBinOp(const RValue &opLeft, const RValue &opRight, BinOp::Type op)
	{
		if constexpr(std::is_arithmetic<T>::value) {
			switch (op) {
				case BinOp::Type::Plus:
					return RValue{opLeft.value<T>() + opRight.value<T>()};
				case BinOp::Type::Minus:
					return RValue{opLeft.value<T>() - opRight.value<T>()};
				case BinOp::Type::Times:
					return RValue{opLeft.value<T>() * opRight.value<T>()};
				case BinOp::Type::Divide:
					return RValue{opLeft.value<T>() / opRight.value<T>()};
			}
		}

		if constexpr(std::is_integral<T>::value) {
			if (op == BinOp::Type::Modulo)
				return RValue{opLeft.value<T>() % opRight.value<T>()};
		}

		std::cerr << "Unable to execute binary operation: " << BinOp::toString(op) << " for type " << typeid(T).name() << '\n';
		abort();
		return RValue{};
	}

	template <typename T>
	static RValue executeUnOp(const RValue &operand, UnOp::Type op)
	{
		if constexpr(std::is_arithmetic<T>::value) {
			if (op == UnOp::Type::Negate)
				return RValue{-operand.value<T>()};

		} else if constexpr(std::is_same<T, bool>::value) {
			if (op == UnOp::Type::Not)
				return RValue{!operand.value<T>()};
		}

		std::cerr << "Unary operation " << UnOp::toString(op) << " not possible for type " << typeid(T).name() << '\n';
		abort();
		return RValue{};
	}


	RValue() : m_type{Type::Immediate}, m_valueType{ValueType::Invalid} {}
	explicit RValue(bool v) : m_type{Type::Immediate}, m_valueType{ValueType::Boolean}, m_value{v} {}
	RValue(int v) : m_type{Type::Immediate}, m_valueType{ValueType::Integer}, m_value{v} {}
	RValue(double v) : m_type{Type::Immediate}, m_valueType{ValueType::Real}, m_value{v} {}
	RValue(const std::string &v) : m_type{Type::Immediate}, m_valueType{ValueType::String}, m_value{v} {}
	RValue(std::string &&v) : m_type{Type::Immediate}, m_valueType{ValueType::String}, m_value{std::move(v)} {}
	RValue(fn_ptr v) : m_type{Type::Immediate}, m_valueType{ValueType::Function}, m_value{v} {}

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
	std::variant <bool, int, double, std::string, void *, fn_ptr> m_value;
};

void matchTypes(RValue &leftRValue, RValue &rightRValue);
std::ostream & operator << (std::ostream &os, const RValue &rv);
