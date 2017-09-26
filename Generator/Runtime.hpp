#pragma once

enum __runcall_operation : int {
	RUNCALL_SCOPE_PUSH,
	RUNCALL_SCOPE_POP,
	RUNCALL_PUSH,
	RUNCALL_VARIABLE_UNSET,
	RUNCALL_VARIABLE_SET,
	RUNCALL_BINOP,
	RUNCALL_FUNCTION_CALL,
};

void runcall(int call, void *arg);
