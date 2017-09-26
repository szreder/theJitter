#pragma once

enum __runcall_operation : int {
	RUNCALL_SCOPE_PUSH,
	RUNCALL_SCOPE_POP,
	RUNCALL_PUSH_RVALUE,
	RUNCALL_VARIABLE_NAME,
	RUNCALL_VARIABLE_UNSET,
	RUNCALL_VARIABLE_SET,
	RUNCALL_BINOP,
};

void runcall(int call, void *arg);
