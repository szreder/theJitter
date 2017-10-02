#pragma once

typedef int RuncallNum;

enum __runcall_operation : RuncallNum {
	RUNCALL_SCOPE_PUSH,
	RUNCALL_SCOPE_POP,
	RUNCALL_PUSH,
	RUNCALL_INIT_VARIABLE,
	RUNCALL_RESOLVE_NAME,
	RUNCALL_ASSIGN,
	RUNCALL_UNOP,
	RUNCALL_BINOP,
	RUNCALL_FUNCTION_CALL,
	RUNCALL_TABLE_CTOR,
	RUNCALL_TABLE_ACCESS,
};

class Program;

void initRuntime(Program &program);
void runcall(RuncallNum call, void *arg);
