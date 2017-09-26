#pragma once

#include <libgccjit.h>
#include <memory>
#include <vector>

#include "Generator/RValue.hpp"
#include "Generator/ValueType.hpp"
#include "Util/EnumHelpers.hpp"

class Program {
public:
	static Program & getInstance()
	{
		static Program instance;
		return instance;
	}

	Program();

	gcc_jit_result * compile() const;
	gcc_jit_context * context() { return m_jitCtx.get(); }
	gcc_jit_function * main() { return m_mainFunc; }
	gcc_jit_rvalue * runtimeCallPtr() { return m_runcallPtr; }
	gcc_jit_type * type(ValueType t) const;

	RValue * allocRValue(const RValue &src = RValue{});
private:
	void prepareTypes();
	void releaseMemory();

	std::unique_ptr <gcc_jit_context, decltype(&gcc_jit_context_release)> m_jitCtx;

	std::array <gcc_jit_type *, toUnderlying(ValueType::_last)> m_basicTypes;
	gcc_jit_type *m_runcallPtrType;
	gcc_jit_rvalue *m_runcallPtr;
	gcc_jit_function *m_mainFunc;

	std::vector <std::unique_ptr <RValue> > m_rvaluePool;
};
