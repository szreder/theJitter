%{

#include "Generator/AST.hpp"
#include "Parser.hpp"

%}

%option noyywrap

DIGIT [0-9]
ID [a-zA-Z_][a-zA-Z0-9_]*

%%

break {
	return BREAK;
}

return {
	return RETURN;
}

nil {
	return NIL;
}

true {
	return TRUE;
}

false {
	return FALSE;
}

\"[^\"]*\"|\'[^\']*\' {
	yylval.str = yytext;
	return STRING_VALUE;
}

{DIGIT}+ {
	yylval.int_value = atoi(yytext);
	return INT_VALUE;
}

{DIGIT}*[.]{DIGIT}+|{DIGIT}+[.]{DIGIT}* {
	yylval.real_value = atof(yytext);
	return REAL_VALUE;
}

"," {
	return COMMA;
}

"=" {
	return ASSIGN;
}

"or" {
	return OR;
}

"and" {
	return AND;
}

"==" {
	return EQ;
}

"~=" {
	return NE;
}

"<" {
	return LT;
}

"<=" {
	return LE;
}

">" {
	return GT;
}

">=" {
	return GE;
}

".." {
	return CONCAT;
}

"+" {
	return PLUS;
}

"-" {
	return MINUS;
}

"*" {
	return TIMES;
}

"/" {
	return DIV;
}

"%" {
	return MOD;
}

"#" {
	return LENGTH;
}

"not" {
	return NOT;
}

"("|")"|"["|"]"|"{"|"}"|"." {
	return yytext[0];
}

{ID} {
	yylval.str = strdup(yytext);
	return ID;
}

[ \t\n\r]+

<<EOF>> {
	return END_OF_INPUT;
}

%%
