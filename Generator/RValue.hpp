#pragma once

#include <libgccjit.h>
#include <variant>

#include "Generator/AST.hpp"
#include "Generator/Variable.hpp"

class Table;
class Variable;

class RValue {
public:
	enum class Type {
		Immediate,
		LValue,
		Temporary,
		_last,
	};

	template <typename T>
	static RValue executeUnOp(const RValue &operand, Lua::UnOp::Type op)
	{
		RValue result{operand};
		result.executeUnOp<T>(op);
		return result;
	}

	template <typename T>
	static RValue executeBinOp(const RValue &opLeft, const RValue &opRight, Lua::BinOp::Type op)
	{
		RValue result{opLeft};
		result.executeBinOp<T>(opRight, op);
		return result;
	}

	template <typename T>
	void executeUnOp(Lua::UnOp::Type op)
	{
		if constexpr(std::is_arithmetic<T>::value) {
			if (op == Lua::UnOp::Type::Negate)
				m_value.second = -std::get<T>(m_value.second);
		} else if constexpr(std::is_same<T, bool>::value) {
			if (op == Lua::UnOp::Type::Not)
				m_value.second = !std::get<T>(m_value.second);
		} else {
			std::cerr << "Unary operation " << Lua::UnOp::toString(op) << " not possible for type " << typeid(T).name() << '\n';
			abort();
		}
	}

	template <typename T>
	void executeBinOp(const RValue &operand, Lua::BinOp::Type op)
	{
		bool ok = true;

		if constexpr(std::is_arithmetic<T>::value) {
			switch (op) {
				case Lua::BinOp::Type::Plus:
					m_value.second = std::get<T>(m_value.second) + operand.value<T>();
					break;
				case Lua::BinOp::Type::Minus:
					m_value.second = std::get<T>(m_value.second) - operand.value<T>();
					break;
				case Lua::BinOp::Type::Times:
					m_value.second = std::get<T>(m_value.second) * operand.value<T>();
					break;
				case Lua::BinOp::Type::Divide:
					m_value.second = std::get<T>(m_value.second) / operand.value<T>();
					break;
				default:
					ok = false;
					break;
			}
		}

		if constexpr(std::is_integral<T>::value) {
			if (!ok && op == Lua::BinOp::Type::Modulo)
				m_value.second = std::get<T>(m_value.second) % operand.value<T>();
		}

		if constexpr(std::is_same<T, bool>::value) {
			if (!ok && op == Lua::BinOp::Type::Plus)
				m_value.second = std::get<T>(m_value.second) + operand.value<T>();
		}

		if (!ok) {
			std::cerr << "Unable to execute binary operation: " << Lua::BinOp::toString(op) << " for type " << typeid(T).name() << '\n';
			abort();
		}
	}

	RValue() : m_type{Type::Immediate}, m_value{ValueType::Invalid, false} {}
	explicit RValue(bool v) : m_type{Type::Immediate}, m_value{ValueType::Boolean, v} {}
	RValue(int v) : m_type{Type::Immediate}, m_value{ValueType::Integer, v} {}
	RValue(double v) : m_type{Type::Immediate}, m_value{ValueType::Real, v} {}
	RValue(const std::string &v) : m_type{Type::Immediate}, m_value{ValueType::String, v} {}
	RValue(std::string &&v) : m_type{Type::Immediate}, m_value{ValueType::String, std::move(v)} {}
	RValue(fn_ptr v) : m_type{Type::Immediate}, m_value{ValueType::Function, v} {}
	RValue(std::shared_ptr <Table> table) : m_type{Type::Immediate}, m_value{ValueType::Table, table} {}

	~RValue() = default;

	static const RValue & Nil()
	{
		static const RValue NilValue = [] {
			RValue result;
			result.m_type = Type::Immediate;
			result.m_value.first = ValueType::Nil;
			return result;
		}();
		return NilValue;
	}

	Type type() const { return m_type; }
	void setType(Type t) { m_type = t; }

	bool isNil() const { return m_value.first == ValueType::Nil; }
	void setNil() { m_type = Type::Immediate; m_value.first = ValueType::Nil; }

	ValueType valueType() const { return m_value.first; }
	void setValueType(ValueType vt) { m_value.first = vt; }

	template <typename T>
	const T & value() const { return std::get<T>(m_value.second); }

	const Value & value() const { return m_value; }
	Value & value() { return m_value; }

	template <typename T>
	void setValue(const T &v) { m_type = Type::Temporary; m_value.second = v; }
	void setValue(const Value &v) { m_type = Type::Temporary; m_value = v; }

	Value * lvalue() { return m_lvalue; }
	void setLValue(Value *lvalue) { m_type = Type::LValue; m_lvalue = lvalue; m_value = *m_lvalue; }

private:
	Type m_type;
	Value *m_lvalue;
	Value m_value;
};

void matchTypes(RValue &leftRValue, RValue &rightRValue);
std::ostream & operator << (std::ostream &os, const RValue &rv);
