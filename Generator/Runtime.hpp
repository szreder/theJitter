#pragma once

typedef int RuncallNum;

enum __runcall_operation : RuncallNum {
	RUNCALL_SCOPE_PUSH,
	RUNCALL_SCOPE_POP,
	RUNCALL_PUSH,
	RUNCALL_VARIABLE_UNSET,
	RUNCALL_VARIABLE_SET,
	RUNCALL_BINOP,
	RUNCALL_FUNCTION_CALL,
};

class Program;

void initRuntime(Program &program);
void runcall(RuncallNum call, void *arg);
