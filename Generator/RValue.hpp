#pragma once

#include <libgccjit.h>
#include <variant>

#include "Generator/AST.hpp"
#include "Generator/Variable.hpp"

class Table;

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

	template <typename T>
	static RValue executeBinOp(const RValue &opLeft, const RValue &opRight, Lua::BinOp::Type op)
	{
		if constexpr(std::is_arithmetic<T>::value) {
			switch (op) {
				case Lua::BinOp::Type::Plus:
					return RValue{opLeft.value<T>() + opRight.value<T>()};
				case Lua::BinOp::Type::Minus:
					return RValue{opLeft.value<T>() - opRight.value<T>()};
				case Lua::BinOp::Type::Times:
					return RValue{opLeft.value<T>() * opRight.value<T>()};
				case Lua::BinOp::Type::Divide:
					return RValue{opLeft.value<T>() / opRight.value<T>()};
			}
		}

		if constexpr(std::is_integral<T>::value) {
			if (op == Lua::BinOp::Type::Modulo)
				return RValue{opLeft.value<T>() % opRight.value<T>()};
		}

		std::cerr << "Unable to execute binary operation: " << Lua::BinOp::toString(op) << " for type " << typeid(T).name() << '\n';
		abort();
		return RValue{};
	}

	template <typename T>
	static RValue executeUnOp(const RValue &operand, Lua::UnOp::Type op)
	{
		if constexpr(std::is_arithmetic<T>::value) {
			if (op == Lua::UnOp::Type::Negate)
				return RValue{-operand.value<T>()};

		} else if constexpr(std::is_same<T, bool>::value) {
			if (op == Lua::UnOp::Type::Not)
				return RValue{!operand.value<T>()};
		}

		std::cerr << "Unary operation " << Lua::UnOp::toString(op) << " not possible for type " << typeid(T).name() << '\n';
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
	RValue(std::shared_ptr <Table> table) : m_type{Type::Immediate}, m_valueType{ValueType::Table}, m_value{table} {}
	RValue(const Variable *var) : m_type{Type::Immediate}, m_valueType{var->type()}, m_value{var->value()} {}

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

	bool isNil() const { return m_valueType == ValueType::Nil; }

	template <typename T>
	const T & value() const { return std::get<T>(m_value); }

	const ValueVariant value() const { return m_value; }

	template <typename T>
	void setValue(const T &v) { m_value = v; }

private:
	Type m_type;
	ValueType m_valueType;
	ValueVariant m_value;
};

void matchTypes(RValue &leftRValue, RValue &rightRValue);
std::ostream & operator << (std::ostream &os, const RValue &rv);
