#pragma once

#include <cstring>
#include <iostream>
#include <memory>
#include <string_view>
#include <unordered_set>
#include <variant>

#include "Generator/Table.hpp"
#include "Generator/ValueType.hpp"

typedef void * (*fn_ptr)(void *);
typedef std::variant <bool, int, double, std::string, void *, fn_ptr, std::shared_ptr <Table> > ValueVariant;

struct Variable {
	ValueType type;
	const char *name;
	ValueVariant value;
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
