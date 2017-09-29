#include <iostream>

#include "Generator/Builtins.hpp"
#include "Util/PrettyPrint.hpp"

inline const __arg_vec * getArgs(void *p)
{
	return reinterpret_cast<const __arg_vec *>(p);
}

void * __ping(void *)
{
	std::cout << "pong\n";
	return nullptr;
}

void * print(void *__args)
{
	auto args = getArgs(__args);

	auto doPrint = [](const RValue &val) {
		switch (val.valueType()) {
			case ValueType::Integer:
				std::cout << val.value<int>();
				break;
			case ValueType::Real:
				std::cout << val.value<double>();
				break;
			case ValueType::Boolean:
				std::cout << std::boolalpha << val.value<bool>();
				break;
			case ValueType::String:
				std::cout << val.value<std::string>();
				break;
			default:
				std::cout << '<' << prettyPrint(val.valueType()) << '>';
				if (val.valueType() == ValueType::Table)
					std::cout << " addr = " << val.value<std::shared_ptr <Table> >();
				break;
		}
	};

	if (!args->empty())
		doPrint((*args)[0]);

	for (size_t i = 1; i != args->size(); ++i) {
		std::cout << ", ";
		doPrint((*args)[i]);
	}

	std::cout << '\n';

	return nullptr;
}
