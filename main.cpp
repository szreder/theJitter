#include <functional>

#include "Generator/AST.hpp"
#include "grammar.tab.h"

extern Node *root;

#include "Generator/Generator.hpp"
#include "Generator/Program.hpp"
#include "Generator/Runtime.hpp"

template <typename T>
std::function<T> function_cast(void *p)
{
	return reinterpret_cast<T *>(p);
}

int main()
{
	yyparse();
	root->print();
	generate(root);
	auto result = Program::getInstance().compile();

	void *fn_ptr = gcc_jit_result_get_code(result, "__main");

	typedef void (*RuntimePtr)(int, void *);
	auto entryPoint = function_cast<void (RuntimePtr)>(fn_ptr);
	entryPoint(runcall);
	std::cerr << " --------------------\n";
	entryPoint(runcall);

	return 0;
}
