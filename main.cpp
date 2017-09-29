#include "Generator/AST.hpp"
extern Lua::Node *root;

#include "Generator/Generator.hpp"
#include "Generator/Program.hpp"
#include "Generator/Runtime.hpp"
#include "Util/Casts.hpp"
#include "Parser.hpp"

int main()
{
	yyparse();
	root->print();
	Lua::generate(root);
	auto result = Program::getInstance().compile();

	void *fn_ptr = gcc_jit_result_get_code(result, "__main");

	typedef void (*RuntimePtr)(int, void *);
	auto entryPoint = function_cast<void (RuntimePtr)>(fn_ptr);
	initRuntime(Program::getInstance());
	entryPoint(runcall);

	return 0;
}
