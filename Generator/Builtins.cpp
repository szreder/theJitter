#include <iostream>

#include "Generator/Builtins.hpp"
#include "Util/PrettyPrint.hpp"

inline const __arg_vec * args_cast(void *p)
{
	return reinterpret_cast<const __arg_vec *>(p);
}

inline RValue * result_cast(void *p)
{
	return reinterpret_cast<RValue *>(p);
}

void __ping(void *__args, void *__result)
{
	auto args = args_cast(__args);
	auto result = result_cast(__result);
	std::cout << "pong\n";
	if (args->empty())
		*result = RValue::Nil();
	else
		*result = (*args)[0];
}

void print(void *__args, void *__result)
{
	auto args = args_cast(__args);
	auto result = result_cast(__result);

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

	*result = RValue::Nil();
}
