#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "Generator/ValueType.hpp"
#include "Util/EnumHelpers.hpp"

class Node {
public:
	enum class Type {
		Chunk,
		ExprList,
		VarList,
		Variable,
		FunctionCall,
		Assignment,
		Value,
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
	virtual bool isBinOp() const { return false; }
	virtual bool isUnOp() const { return false; }

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
	~Chunk()
	{
		for (auto p : m_children)
			delete p;
	}

	void append(Node *n) override
	{
		m_children.push_back(n);
	}

	const std::vector <Node *> & children() const
	{
		return m_children;
	}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Chunk:\n";
		for (const Node *n : m_children)
			n->print(indent + 1);
	}

	Node::Type type() const override { return Type::Chunk; }

private:
	std::vector <Node *> m_children;
};

class ExprList : public Node {
public:
	~ExprList()
	{
		for (auto p : m_exprs)
			delete p;
	}

	void append(Node *n) override
	{
		m_exprs.push_back(n);
	}

	const std::vector <Node *> & exprs() const
	{
		return m_exprs;
	}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Expression list: [\n";
		for (const Node *n : m_exprs)
			n->print(indent + 1);
		do_indent(indent);
		std::cout << "]\n";
	}

	Node::Type type() const override { return Type::ExprList; }

private:
	std::vector <Node *> m_exprs;
};

class VarList : public Node {
public:
	void append(const char *s)
	{
		std::cout << "VarList::append(" << s << ")\n";
		m_vars.emplace_back(s);
	}

	const std::vector <std::string> & vars() const
	{
		return m_vars;
	}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Variable list: [\n";
		bool first = true;
		for (const std::string &var : m_vars) {
			if (!first)
				std::cout << ",\n";
			first = false;
			do_indent(indent + 1);
			std::cout << var;
		}
		std::cout << '\n';
		do_indent(indent);
		std::cout << "]\n";
	}

	Node::Type type() const override { return Type::VarList; }

private:
	std::vector <std::string> m_vars;
};

class Assignment : public Node {
public:
	constexpr Assignment(VarList *vl, ExprList *el) : m_varList{vl}, m_exprList{el} {}
	~Assignment()
	{
		delete m_varList;
		delete m_exprList;
	}

	const VarList * varList() const
	{
		return m_varList;
	}

	const ExprList * exprList() const
	{
		return m_exprList;
	}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Assignment:\n";
		m_varList->print(indent + 1);
		m_exprList->print(indent + 1);
	}

	Node::Type type() const override { return Type::Assignment; }

private:
	VarList *m_varList;
	ExprList *m_exprList;
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
	FunctionCall(Node *funcExpr) : m_functionExpr{funcExpr} {}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Function call:\n";
		m_functionExpr->print(indent + 1);
		do_indent(indent);
		std::cout << "Args:\n";
	}

	void setArgs(ExprList *args) { m_args = args; }

	Node::Type type() const override { return Type::FunctionCall; }
private:
	Node *m_functionExpr;
	ExprList *m_args;
};

class VarNode : public Node {
public:
	VarNode(const char *varname) : m_variableName{varname} {}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Var: " << m_variableName << '\n';
	}

	const std::string & name() const { return m_variableName; }

	Node::Type type() const override { return Type::Variable; }
private:
	std::string m_variableName;
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

	constexpr Field(Node *expr, Node *val) : m_type{Type::Brackets}, m_expr{expr}, m_value{val} {}
	Field(const std::string &s, Node *val) : m_type{Type::Literal}, m_fieldName{s}, m_value{val} {}
	constexpr Field( Node *val) : m_type{Type::NoIndex}, m_expr{nullptr}, m_value{val} {}

	~Field()
	{
		if (m_type == Type::Brackets)
			delete m_expr;
		delete m_value;
	}

	void print(int indent) const override
	{
		do_indent(indent);
		switch (m_type) {
			case Type::Brackets:
				std::cout << "Expr to expr:\n";
				m_expr->print(indent + 1);
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

		m_value->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::Field; }

private:
	Type m_type;
	union {
		Node *m_expr;
		std::string m_fieldName;
	};
	Node *m_value;
};

class TableValue : public Value {
public:
	~TableValue()
	{
		for (auto p : m_fields)
			delete p;
	}

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "Table:\n";
		for (const auto &p : m_fields)
			p->print(indent + 1);
	}

	ValueType valueType() const override { return ValueType::Table; }

	void addField(Field *f)
	{
		m_fields.push_back(f);
	}
private:
	std::vector <Field *> m_fields;
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

	constexpr BinOp(Type t, Node *left, Node *right) : m_type{t}, m_left{left}, m_right{right} {}

	~BinOp()
	{
		delete m_left;
		delete m_right;
	}

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

	bool isBinOp() const override { return true; }
	Type binOpType() const { return m_type; }

	const Node * left() const { return m_left; }
	const Node * right() const { return m_right; }

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "BinOp: " << toString() << '\n';
		m_left->print(indent + 1);
		m_right->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::BinOp; }

	const char * toString() const
	{
		return toString(m_type);
	}

private:
	Type m_type;
	Node *m_left, *m_right;
};

class UnOp : public Node {
public:
	enum class Type {
		Negate,
		Not,
		Length,
	};

	constexpr UnOp(Type t, Node *op) : m_type{t}, m_operand{op} {}

	~UnOp()
	{
		delete m_operand;
	}

	bool isUnOp() const override { return true; }

	void print(int indent) const override
	{
		do_indent(indent);
		std::cout << "UnOp: " << toString() << '\n';
		m_operand->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::UnOp; }

private:
	const char * toString() const
	{
		static const char *s[] = {"-", "not", "#"};
		return s[static_cast<int>(m_type)];
	}

	Type m_type;
	Node *m_operand;
};
