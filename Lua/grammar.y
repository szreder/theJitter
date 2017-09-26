%{
#include <iostream>

#include "Generator/AST.hpp"

int yylex();
int yyerror(const char *s)
{
	std::cerr << s << '\n';
}

Node *root;

int yydebug = 1;

%}

%define parse.error verbose

%union {
	Node *node;
	ExprList *expr_list;
	VarList *var_list;
	FunctionCall *func_call;
	double real_value;
	int int_value;
	char *str;
	TableValue *table_value;
	Field *field;
}

%type <node> chunk expr prefix_expr statement
%type <expr_list> args expr_list
%type <func_call> func_call
%type <var_list> var_list
%type <str> var
%type <table_value> field_list table_ctor
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
	$$ = new Chunk{};
	$$->append($1);
}
| chunk statement {
	$$ = $1;
	$$->append($2);
}
;

statement :
var_list ASSIGN expr_list {
	$$ = new Assignment{$1, $3};
}
| func_call {
	$$ = $1;
}
;

expr_list :
expr {
	$$ = new ExprList{};
	$$->append($1);
}
| expr_list COMMA expr {
	$$ = $1;
	$$->append($3);
}

var_list :
var {
	std::cout << "new varlist with ID: " << $1 << '\n';
	$$ = new VarList{};
	$$->append($1);
}
| var_list COMMA var {
	std::cout << "new var with ID: " << $3 << '\n';
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
	$$ = new VarNode{$1};
	free($1);
}
| "(" expr ")" {
	$$ = $2;

}
| func_call {
	$$ = $1;
}

func_call :
prefix_expr args {
	$$ = new FunctionCall{$1};
	$$->setArgs($2);
}

args :
'(' expr_list ')' {
	$$ = $2;
}
| '(' ')' {
	$$ = new ExprList{};
}

expr :
NIL {
	$$ = new NilValue{};
}
| FALSE {
	$$ = new BooleanValue{false};
}
| TRUE {
	$$ = new BooleanValue{true};
}
| INT_VALUE {
	$$ = new IntValue{$1};
}
| REAL_VALUE {
	std::cout << "RealValue: " << $1 << '\n';
	$$ = new RealValue{$1};
}
| STRING_VALUE {
	$$ = new StringValue{$1};
}
| expr PLUS expr {
	$$ = new BinOp{BinOp::Type::Plus, $1, $3};
}
| expr MINUS expr {
	$$ = new BinOp{BinOp::Type::Minus, $1, $3};
}
| expr TIMES expr {
	$$ = new BinOp{BinOp::Type::Times, $1, $3};
}
| expr DIV expr {
	$$ = new BinOp{BinOp::Type::Divide, $1, $3};
}
| expr MOD expr {
	$$ = new BinOp{BinOp::Type::Modulo, $1, $3};
}
| MINUS expr %prec NEGATE {
	$$ = new UnOp{UnOp::Type::Negate, $2};
}
| table_ctor {
	$$ = $1;
}
| prefix_expr {
	$$ = $1;
}
;

table_ctor :
'{' '}' {
	$$ = new TableValue{};
}
| '{' field_list '}' {
	$$ = $2;
}

field_list :
field {
	$$ = new TableValue{};
	$$->append($1);
}
| field_list ',' field {
	$$ = $1;
	$$->append($3);
}

field :
'[' expr ']' '=' expr {
	$$ = new Field{$2, $5};
}
| ID '=' expr {
	$$ = new Field{$1, $3};
}
| expr {
	$$ = new Field{$1};
}

%%
