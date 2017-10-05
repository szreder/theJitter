#include <functional>
#include <iostream>
#include <libgccjit.h>

template <typename T>
std::function<T> ptr_to_fun(void *p)
{
	return reinterpret_cast<T *>(p);
}

/*
 * function sum(n)
 * 	local result = 0
*
 * 	for i = 1, n - 1 do
 * 		result = result + i
 * 	end
 *
 * 	return result
 * end
 */

gcc_jit_context * sum()
{
	gcc_jit_context *ctx;
	ctx = gcc_jit_context_acquire();

	gcc_jit_type *arithmetic_type;
// 	arithmetic_type = gcc_jit_context_get_type(ctx, GCC_JIT_TYPE_DOUBLE);
	arithmetic_type = gcc_jit_context_get_type(ctx, GCC_JIT_TYPE_UNSIGNED_LONG_LONG);

	gcc_jit_param *param_n = gcc_jit_context_new_param(
		ctx, nullptr, arithmetic_type, "n"
	);

	gcc_jit_function *func = gcc_jit_context_new_function(
		ctx, nullptr, GCC_JIT_FUNCTION_EXPORTED, arithmetic_type,
		"sum", 1, &param_n, false
	);

	gcc_jit_block *entry_block = gcc_jit_function_new_block(func, nullptr);

	gcc_jit_lvalue *result = gcc_jit_function_new_local(
		func, nullptr, arithmetic_type, "result"
	);
	gcc_jit_block_add_assignment(
		entry_block, nullptr,
		result,
		gcc_jit_context_zero(ctx, arithmetic_type)
	);

	gcc_jit_lvalue *iterator = gcc_jit_function_new_local(
		func, nullptr, arithmetic_type, "i"
	);
	gcc_jit_block_add_assignment(
		entry_block, nullptr,
		iterator,
		gcc_jit_context_one(ctx, arithmetic_type)
	);

	gcc_jit_block *true_block = gcc_jit_function_new_block(func, nullptr);
	gcc_jit_block *false_block = gcc_jit_function_new_block(func, nullptr);

	gcc_jit_rvalue *cmp = gcc_jit_context_new_comparison(
		ctx, nullptr, GCC_JIT_COMPARISON_LE,
		gcc_jit_lvalue_as_rvalue(iterator),
		gcc_jit_param_as_rvalue(param_n)
	);

	gcc_jit_block_end_with_conditional(
		entry_block, nullptr,
		cmp, true_block, false_block
	);

	gcc_jit_block_add_assignment_op(
		true_block, nullptr,
		result, GCC_JIT_BINARY_OP_PLUS,
		gcc_jit_lvalue_as_rvalue(iterator)
	);

	gcc_jit_block_add_assignment_op(
		true_block, nullptr,
		iterator, GCC_JIT_BINARY_OP_PLUS,
		gcc_jit_context_one(ctx, arithmetic_type)
	);

	gcc_jit_block_end_with_conditional(
		true_block, nullptr,
		cmp, true_block, false_block
	);

	gcc_jit_block_end_with_return(
		false_block, nullptr,
		gcc_jit_lvalue_as_rvalue(result)
	);

	return ctx;
}

int main()
{
	gcc_jit_context *ctx = sum();
	gcc_jit_context_set_int_option(
		ctx, GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, 2
	);
	gcc_jit_result *res = gcc_jit_context_compile(ctx);
	gcc_jit_context_compile_to_file(
		ctx, GCC_JIT_OUTPUT_KIND_ASSEMBLER, "jitcode.s"
	);
	gcc_jit_context_release(ctx);

	void *fn_ptr = gcc_jit_result_get_code(res, "sum");
	if (fn_ptr == nullptr) {
		fprintf(stderr, "JIT failed\n");
		return 1;
	}

// 	auto pfun = ptr_to_fun<double (double)>(fn_ptr);
	auto pfun = ptr_to_fun<unsigned long long (unsigned long long)>(fn_ptr);
	std::cout << pfun(1000 * 1000 * 1000) << '\n';
	return 0;
}
