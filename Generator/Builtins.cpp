#include <iostream>

void * ping(void *)
{
	std::cout << "pong\n";
	return nullptr;
}
