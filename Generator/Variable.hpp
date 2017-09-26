#pragma once

#include <cstring>
#include <iostream>
#include <string_view>
#include <unordered_set>

#include "Generator/ValueType.hpp"

struct Variable {
	ValueType type;
	const char *name;
	union {
		bool b;
		int i;
		double r;
		const char *s;
		void *v;
	};
};

std::ostream & operator << (std::ostream &os, const Variable &v);

struct VariableHasher {
	size_t operator()(const Variable &v) const
	{
		return std::hash<std::string_view>{}(v.name);
	}
};

struct VariableComparer {
	bool operator()(const Variable &a, const Variable &b) const
	{
		return strcmp(a.name, b.name) == 0;
	}
};
