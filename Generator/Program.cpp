#include <algorithm>
#include <array>

#include "Generator/Program.hpp"

Program::Program()
	: m_jitCtx{gcc_jit_context_acquire(), gcc_jit_context_release}
{
	prepareTypes();

	gcc_jit_param *runtime = gcc_jit_context_new_param(m_jitCtx.get(), nullptr, m_runcallPtrType, "__runtime");
	m_mainFunc = gcc_jit_context_new_function(
		m_jitCtx.get(), nullptr, GCC_JIT_FUNCTION_EXPORTED,
		type(ValueType::Nil), "__main", 1, &runtime, 0);

	m_runcallPtr = gcc_jit_param_as_rvalue(runtime);
}

gcc_jit_result * Program::compile() const
{
	gcc_jit_context_set_bool_option(m_jitCtx.get(), GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE, true);
	return gcc_jit_context_compile(m_jitCtx.get());
}

gcc_jit_type * Program::type(ValueType t) const
{
	return m_basicTypes[toUnderlying(t)];
}

RValue * Program::allocRValue(const RValue &src)
{
	m_rvaluePool.push_back(std::make_unique<RValue>(src));
	return m_rvaluePool.back().get();
}

std::string * Program::duplicateString(const char *s)
{
	m_stringPool.push_back(std::make_unique<std::string>(s));
	return m_stringPool.back().get();
}

std::string * Program::duplicateString(const std::string &s)
{
	return duplicateString(s.data());
}

void Program::prepareTypes()
{
	auto ctx = m_jitCtx.get();

	std::fill(m_basicTypes.begin(), m_basicTypes.end(), nullptr);
	m_basicTypes[toUnderlying(ValueType::Nil)] = gcc_jit_context_get_type(ctx, GCC_JIT_TYPE_VOID);
	m_basicTypes[toUnderlying(ValueType::Boolean)] = gcc_jit_context_get_type(ctx, GCC_JIT_TYPE_BOOL);
	m_basicTypes[toUnderlying(ValueType::Integer)] = gcc_jit_context_get_type(ctx, GCC_JIT_TYPE_INT);
	m_basicTypes[toUnderlying(ValueType::Real)] = gcc_jit_context_get_type(ctx, GCC_JIT_TYPE_DOUBLE);
	m_basicTypes[toUnderlying(ValueType::String)] = gcc_jit_context_get_type(ctx, GCC_JIT_TYPE_CONST_CHAR_PTR);
	m_basicTypes[toUnderlying(ValueType::Unknown)] = gcc_jit_context_get_type(ctx, GCC_JIT_TYPE_VOID_PTR);

	gcc_jit_type *runcall_param_types[2] = {
		m_basicTypes[toUnderlying(ValueType::Integer)],
		m_basicTypes[toUnderlying(ValueType::Unknown)],
	};

	m_runcallPtrType = gcc_jit_context_new_function_ptr_type(ctx, nullptr,
		m_basicTypes[toUnderlying(ValueType::Unknown)], 2, runcall_param_types, 0);
}
