#include <functional>
#include <iostream>
#include <libgccjit.h>

template <typename T>
std::function<T> ptr_to_fun(void *p)
{
	return reinterpret_cast<T *>(p);
}

/*
 * function factorial(n)
 * 	if x == 0 then
 * 		return 1
 * 	else
 * 		return n * factorial(n - 1)
 * 	end
 * end
 */

gcc_jit_context * factorial()
{
	gcc_jit_context *ctx;
	ctx = gcc_jit_context_acquire();

	gcc_jit_type *int_type;
	int_type = gcc_jit_context_get_type(ctx, GCC_JIT_TYPE_INT);

	gcc_jit_param *param_n = gcc_jit_context_new_param(
		ctx, nullptr, int_type, "n"
	);

	gcc_jit_function *func = gcc_jit_context_new_function(
		ctx, nullptr, GCC_JIT_FUNCTION_EXPORTED, int_type,
		"factorial", 1, &param_n, false
	);

	gcc_jit_block *entry_block = gcc_jit_function_new_block(func, nullptr);
	gcc_jit_block *true_block = gcc_jit_function_new_block(func, nullptr);
	gcc_jit_block *false_block = gcc_jit_function_new_block(func, nullptr);

	gcc_jit_rvalue *cmp = gcc_jit_context_new_comparison(
		ctx, nullptr, GCC_JIT_COMPARISON_EQ,
		gcc_jit_param_as_rvalue(param_n),
		gcc_jit_context_zero(ctx, int_type)
	);

	gcc_jit_block_end_with_conditional(
		entry_block, nullptr,
		cmp, true_block, false_block
	);

	gcc_jit_block_end_with_return(
		true_block, nullptr,
		gcc_jit_context_one(ctx, int_type)
	);

	gcc_jit_rvalue *n1 = gcc_jit_context_new_binary_op(
		ctx, nullptr, GCC_JIT_BINARY_OP_MINUS, int_type,
		gcc_jit_param_as_rvalue(param_n),
		gcc_jit_context_one(ctx, int_type)
	);

	gcc_jit_rvalue *mul = gcc_jit_context_new_binary_op(
		ctx, nullptr, GCC_JIT_BINARY_OP_MULT, int_type,
		gcc_jit_param_as_rvalue(param_n),
		gcc_jit_context_new_call(
			ctx, nullptr, func, 1, &n1
		)
	);

	gcc_jit_block_end_with_return(
		false_block, nullptr,
		mul
	);

	return ctx;
}

int main()
{
	gcc_jit_context *fac = factorial();
	gcc_jit_result *res = gcc_jit_context_compile(fac);
	gcc_jit_context_release(fac);

	void *fn_ptr = gcc_jit_result_get_code(res, "factorial");
	if (fn_ptr == nullptr) {
		fprintf(stderr, "JIT failed\n");
		return 1;
	}

	auto pfun = ptr_to_fun<int (int)>(fn_ptr);
	std::cout << pfun(5) << '\n';

	return 0;
}
