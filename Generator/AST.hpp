#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "Generator/ValueType.hpp"
#include "Util/EnumHelpers.hpp"

namespace Lua {

class Node {
public:
	enum class Type {
		Chunk,
		ExprList,
		VarList,
		LValue,
		FunctionCall,
		Assignment,
		Value,
		TableCtor,
		Field,
		BinOp,
		UnOp,
		_last,
	};

	virtual void append(Node *n) { assert(false); }

	virtual void print(int indent = 0) const
	{
		do_indent(indent);
		std::cout << "Node\n";
	}

	Node() = default;
	virtual ~Node() = default;
	Node(Node &&) = default;

	virtual bool isValue() const { return false; }

	Node(const Node &) = delete;
	Node & operator = (const Node &) = delete;
	Node & operator = (Node &&) = default;

	virtual Type type() const = 0;

protected:
	void do_indent(int indent) const
	{
		for (int i = 0; i < indent; ++i)
			std::cout << '\t';
	}
};

class Chunk : public Node {
public:
	void append(Node *n) override { m_children.emplace_back(n); }

	const std::vector <std::unique_ptr <Node> > & children() const { return m_children; }

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Chunk:\n";
		for (const auto &n : m_children)
			n->print(indent + 1);
	}

	Node::Type type() const override { return Type::Chunk; }

private:
	std::vector <std::unique_ptr <Node> > m_children;
};

class ExprList : public Node {
public:
	void append(Node *n) override { m_exprs.emplace_back(n); }

	const std::vector <std::unique_ptr <Node> > & exprs() const { return m_exprs; }

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Expression list: [\n";
		for (const auto &n : m_exprs)
			n->print(indent + 1);
		do_indent(indent);
		std::cout << "]\n";
	}

	Node::Type type() const override { return Type::ExprList; }

private:
	std::vector <std::unique_ptr <Node> > m_exprs;
};

class LValue : public Node {
public:
	enum class Type {
		Bracket,
		Dot,
		Name,
	};

	LValue(Node *tableExpr, Node *keyExpr) : m_type{Type::Bracket}, m_tableExpr{tableExpr}, m_keyExpr{keyExpr} {}
	LValue(Node *tableExpr, const char *fieldName) : m_type{Type::Dot}, m_tableExpr{tableExpr}, m_name{fieldName} {}
	LValue(const char *varName) : m_type{Type::Name}, m_name{varName} {}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "LValue";
		switch (m_type) {
			case Type::Bracket:
				std::cout << " bracket operator:\n";
				m_tableExpr->print(indent + 1);
				m_keyExpr->print(indent + 1);
				break;
			case Type::Dot:
				std::cout << " dot operator:\n";
				m_tableExpr->print(indent + 1);
				do_indent(indent + 1);
				std::cout << "Field name: " << m_name << '\n';
				break;
			case Type::Name:
				std::cout << '\n';
				do_indent(indent + 1);
				std::cout << m_name << '\n';
				break;
		}
	}

	const std::string & name() const { return m_name; }
	const Node * tableExpr() const { return m_tableExpr.get(); }
	const Node * keyExpr() const { return m_keyExpr.get(); }

	Type lvalueType() const { return m_type; }
	Node::Type type() const override { return Node::Type::LValue; }
private:
	Type m_type;
	std::unique_ptr <Node> m_tableExpr;
	std::unique_ptr <Node> m_keyExpr;
	std::string m_name;
};

class VarList : public Node {
public:
	void append(LValue *lval)
	{
		m_vars.emplace_back(lval);
	}

	const std::vector <std::unique_ptr <LValue> > & vars() const
	{
		return m_vars;
	}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Variable list: [\n";
		for (const auto &lv : m_vars) {
			lv->print(indent + 1);
		}
		do_indent(indent);
		std::cout << "]\n";
	}

	Node::Type type() const override { return Type::VarList; }

private:
	std::vector <std::unique_ptr <LValue> > m_vars;
};

class Assignment : public Node {
public:
	Assignment(VarList *vl, ExprList *el) : m_varList{vl}, m_exprList{el} {}

	const VarList * varList() const { return m_varList.get(); }

	const ExprList * exprList() const { return m_exprList.get(); }

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Assignment:\n";
		m_varList->print(indent + 1);
		m_exprList->print(indent + 1);
	}

	Node::Type type() const override { return Type::Assignment; }

private:
	std::unique_ptr <VarList> m_varList;
	std::unique_ptr <ExprList> m_exprList;
};

class Value : public Node {
public:
	bool isValue() const override { return true; }

	Node::Type type() const override { return Type::Value; }

	virtual ValueType valueType() const = 0;
};

class NilValue : public Value {
public:
	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "nil\n";
	}

	ValueType valueType() const override { return ValueType::Nil; }
};

class BooleanValue : public Value {
public:
	BooleanValue(bool v) : m_value{v} {}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << std::boolalpha << m_value << '\n';
	}

	ValueType valueType() const override { return ValueType::Boolean; }

	bool value() const { return m_value; }
private:
	bool m_value;
};

class StringValue : public Value {
public:
	StringValue(const char *v) : m_value{v} {}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "String: " << m_value << '\n';
	}

	ValueType valueType() const override { return ValueType::String; }

	const std::string & value() const { return m_value; }
private:
	std::string m_value;
};

class FunctionCall : public Node {
public:
	FunctionCall(Node *funcExpr, ExprList *args) : m_functionExpr{funcExpr}, m_args{args} {}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Function call:\n";
		m_functionExpr->print(indent + 1);
		do_indent(indent);
		std::cout << "Args:\n";
		m_args->print(indent + 1);
	}

	const Node * functionExpr() const { return m_functionExpr.get(); }

	const ExprList * args() const { return m_args.get(); }

	Node::Type type() const override { return Type::FunctionCall; }
private:
	std::unique_ptr <Node> m_functionExpr;
	std::unique_ptr <ExprList> m_args;
};

class IntValue : public Value {
public:
	constexpr IntValue(int v) : m_value{v} {}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Int: " << m_value << '\n';
	}

	ValueType valueType() const override { return ValueType::Integer; }

	int value() const { return m_value; }
private:
	int m_value;
};

class RealValue : public Value {
public:
	constexpr RealValue(double v) : m_value{v} {}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Real: " << m_value << '\n';
	}

	ValueType valueType() const override { return ValueType::Real; }

	double value() const { return m_value; }
private:
	double m_value;
};

class Field : public Node {
public:
	enum class Type {
		Brackets,
		Literal,
		NoIndex,
	};

	Field(Node *expr, Node *val) : m_type{Type::Brackets}, m_keyExpr{expr}, m_valueExpr{val} {}
	Field(const std::string &s, Node *val) : m_type{Type::Literal}, m_fieldName{s}, m_valueExpr{val} {}
	Field(Node *val) : m_type{Type::NoIndex}, m_keyExpr{nullptr}, m_valueExpr{val} {}

	void print(int indent) const override
	{
		do_indent(indent);
		switch (m_type) {
			case Type::Brackets:
				std::cout << "Expr to expr:\n";
				m_keyExpr->print(indent + 1);
				break;
			case Type::Literal:
				std::cout << "Name to expr:\n";
				do_indent(indent + 1);
				std::cout << m_fieldName << '\n';
				break;
			case Type::NoIndex:
				std::cout << "Expr:\n";
				break;
		}

		m_valueExpr->print(indent + 1);
	}

	Type fieldType() const { return m_type; }

	Node::Type type() const override { return Node::Type::Field; }

	const std::string & fieldName() const { return m_fieldName; }
	const Node * keyExpr() const { return m_keyExpr.get(); }
	const Node * valueExpr() const { return m_valueExpr.get(); }

private:
	Type m_type;
	std::string m_fieldName;
	std::unique_ptr <Node> m_keyExpr;
	std::unique_ptr <Node> m_valueExpr;
};

class TableCtor : public Node {
public:
	void append(Field *f) { m_fields.emplace_back(f); }

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Table:\n";
		for (const auto &p : m_fields)
			p->print(indent + 1);
	}

	const std::vector <std::unique_ptr <Field> > & fields() const { return m_fields; }

	Node::Type type() const override { return Node::Type::TableCtor; }
private:
	std::vector <std::unique_ptr <Field> > m_fields;
};

class BinOp : public Node {
public:
	enum class Type {
		Or,
		And,
		Equals,
		NotEqual,
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		Concat,
		Plus,
		Minus,
		Times,
		Divide,
		Modulo,
		_last
	};

	BinOp(Type t, Node *left, Node *right) : m_type{t}, m_left{left}, m_right{right} {}

	static const std::vector <ValueType> & applicableTypes(Type t)
	{
		static auto ApplicableTypes = []{
			std::array <std::vector <ValueType>, toUnderlying(Type::_last)> result;

			for (auto v : {Type::Plus, Type::Minus, Type::Times, Type::Divide})
				result[toUnderlying(v)] = {ValueType::Integer, ValueType::Real};

			result[toUnderlying(Type::Modulo)] = {ValueType::Integer};

			return result;
		}();

		return ApplicableTypes[toUnderlying(t)];
	}

	static bool isApplicable(Type t, ValueType vt)
	{
		auto types = applicableTypes(t);
		return std::find(types.begin(), types.end(), vt) != types.end();
	}

	static const char * toString(Type t)
	{
		static const char *s[] = {"or", "and", "==", "~=", "<", "<=", ">", ">=", "..", "+", "-", "*", "/", "%"};
		return s[toUnderlying(t)];
	}

	Type binOpType() const { return m_type; }

	const Node * left() const { return m_left.get(); }
	const Node * right() const { return m_right.get(); }

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "BinOp: " << toString() << '\n';
		m_left->print(indent + 1);
		m_right->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::BinOp; }

	const char * toString() const { return toString(m_type); }

private:
	Type m_type;
	std::unique_ptr <Node> m_left;
	std::unique_ptr <Node> m_right;
};

class UnOp : public Node {
public:
	enum class Type {
		Negate,
		Not,
		Length,
	};

	UnOp(Type t, Node *op) : m_type{t}, m_operand{op} {}

	static const char * toString(Type t)
	{
		static const char *s[] = {"-", "not", "#"};
		return s[toUnderlying(t)];
	}

	Type unOpType() const { return m_type; }

	Node * operand() const { return m_operand.get(); }

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "UnOp: " << toString() << '\n';
		m_operand->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::UnOp; }

	const char * toString() const { return toString(m_type); }

private:
	Type m_type;
	std::unique_ptr <Node> m_operand;
};

} //namespace Lua
