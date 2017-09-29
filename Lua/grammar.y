%{
#include <iostream>

#include "Generator/AST.hpp"

int yylex();
int yyerror(const char *s)
{
	std::cerr << s << '\n';
}

Lua::Node *root;

int yydebug = 1;

%}

%define parse.error verbose

%union {
	int int_value;
	char *str;
	double real_value;
	Lua::Node *node;
	Lua::ExprList *expr_list;
	Lua::VarList *var_list;
	Lua::FunctionCall *func_call;
	Lua::TableCtor *table;
	Lua::Field *field;
}

%type <node> chunk expr prefix_expr statement
%type <expr_list> args expr_list
%type <func_call> func_call
%type <var_list> var_list
%type <str> var
%type <table> field_list table_ctor
%type <field> field

%token <int_value> INT_VALUE
%token <real_value> REAL_VALUE
%token <str> ID STRING_VALUE
%token BREAK RETURN NIL TRUE FALSE
%token LENGTH NOT
%token END_OF_INPUT 0 "eof"

%right ASSIGN
%left COMMA
%left OR
%left AND
%left EQ NE LT LE GT GE
%right CONCAT
%left PLUS MINUS
%left TIMES DIV MOD
%precedence NEGATE

%%

root :
chunk {
	root = $1;
}

chunk :
statement {
	$$ = new Lua::Chunk{};
	$$->append($1);
}
| chunk statement {
	$$ = $1;
	$$->append($2);
}
;

statement :
var_list ASSIGN expr_list {
	$$ = new Lua::Assignment{$1, $3};
}
| func_call {
	$$ = $1;
}
;

expr_list :
expr {
	$$ = new Lua::ExprList{};
	$$->append($1);
}
| expr_list COMMA expr {
	$$ = $1;
	$$->append($3);
}

var_list :
var {
	$$ = new Lua::VarList{};
	$$->append($1);
}
| var_list COMMA var {
	$$ = $1;
	$$->append($3);
}

var :
ID {
	$$ = $1;
}
/*
| prefix_expr "[" expr "]" {
	$$ =
}
| prefix_expr "." ID {

}
*/

prefix_expr :
var {
	$$ = new Lua::VarNode{$1};
	free($1);
}
| '(' expr ')' {
	$$ = $2;

}
| func_call {
	$$ = $1;
}

func_call :
prefix_expr args {
	$$ = new Lua::FunctionCall{$1};
	$$->setArgs($2);
}

args :
'(' expr_list ')' {
	$$ = $2;
}
| '(' ')' {
	$$ = new Lua::ExprList{};
}

expr :
NIL {
	$$ = new Lua::NilValue{};
}
| FALSE {
	$$ = new Lua::BooleanValue{false};
}
| TRUE {
	$$ = new Lua::BooleanValue{true};
}
| INT_VALUE {
	$$ = new Lua::IntValue{$1};
}
| REAL_VALUE {
	$$ = new Lua::RealValue{$1};
}
| STRING_VALUE {
	$$ = new Lua::StringValue{$1};
}
| expr PLUS expr {
	$$ = new Lua::BinOp{Lua::BinOp::Type::Plus, $1, $3};
}
| expr MINUS expr {
	$$ = new Lua::BinOp{Lua::BinOp::Type::Minus, $1, $3};
}
| expr TIMES expr {
	$$ = new Lua::BinOp{Lua::BinOp::Type::Times, $1, $3};
}
| expr DIV expr {
	$$ = new Lua::BinOp{Lua::BinOp::Type::Divide, $1, $3};
}
| expr MOD expr {
	$$ = new Lua::BinOp{Lua::BinOp::Type::Modulo, $1, $3};
}
| MINUS expr %prec NEGATE {
	$$ = new Lua::UnOp{Lua::UnOp::Type::Negate, $2};
}
| table_ctor {
	std::cout << "table ctor\n";
	$$ = $1;
}
| prefix_expr {
	$$ = $1;
}
;

table_ctor :
'{' '}' {
	std::cout << "empty table ctor\n";
	$$ = new Lua::TableCtor{};
}
| '{' field_list '}' {
	std::cout << "field table ctor\n";
	$$ = $2;
}

field_list :
field {
	std::cout << "new Lua::TableCtorValue\n";
	$$ = new Lua::TableCtor{};
	$$->append($1);
}
| field_list COMMA field {
	$$ = $1;
	$$->append($3);
}

field :
'[' expr ']' ASSIGN expr {
	$$ = new Lua::Field{$2, $5};
}
| ID ASSIGN expr {
	$$ = new Lua::Field{$1, $3};
}
| expr {
	$$ = new Lua::Field{$1};
}

%%
