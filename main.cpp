#include "Generator/AST.hpp"
#include "grammar.tab.h"

extern Node *root;

#include "Generator/Generator.hpp"
#include "Generator/Program.hpp"
#include "Generator/Runtime.hpp"
#include "Util/Casts.hpp"

int main()
{
	yyparse();
	root->print();
	generate(root);
	auto result = Program::getInstance().compile();

	void *fn_ptr = gcc_jit_result_get_code(result, "__main");

	typedef void (*RuntimePtr)(int, void *);
	auto entryPoint = function_cast<void (RuntimePtr)>(fn_ptr);
	initRuntime(Program::getInstance());
	entryPoint(runcall);

	return 0;
}
