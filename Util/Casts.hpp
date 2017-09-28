#pragma once

#include <functional>

template <typename T>
constexpr void * toVoidPtr(T v)
{
	return reinterpret_cast<void *>(static_cast<uintptr_t>(v));
}

template <typename T>
constexpr T fromVoidPtr(void *v)
{
	return static_cast<T>(reinterpret_cast<uintptr_t>(v));
}

template <typename T>
std::function<T> function_cast(void *p)
{
	return reinterpret_cast<T *>(p);
}
