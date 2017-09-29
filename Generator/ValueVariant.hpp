#pragma once

#include <memory>
#include <string>
#include <variant>

class Table;

typedef void * (*fn_ptr)(void *);
typedef std::variant <bool, int, double, std::string, void *, fn_ptr, std::shared_ptr <Table> > ValueVariant;
