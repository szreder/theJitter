#pragma once

#include <type_traits>

template <typename ET>
constexpr typename std::underlying_type <ET>::type toUnderlying(ET et)
{
	return static_cast<typename std::underlying_type<ET>::type>(et);
}

template <typename ET>
constexpr typename std::underlying_type <ET>::type & toUnderlyingRef(ET &et)
{
	return reinterpret_cast<typename std::underlying_type<ET>::type &>(et);
}

template <typename ET>
constexpr void * toVoidPtr(ET et)
{
	return reinterpret_cast<void *>(static_cast<uintptr_t>(toUnderlying(et)));
}

template <typename ET>
constexpr ET fromVoidPtr(void *v)
{
	return static_cast<ET>(reinterpret_cast<uintptr_t>(v));
}
