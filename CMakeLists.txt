project(theJitter)
cmake_minimum_required(VERSION 2.8)

find_package(BISON)
find_package(FLEX)

set(CMAKE_CXX_FLAGS "-Wall -std=c++17 -ggdb")

bison_target(Parser Lua/grammar.y ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp)
flex_target(Scanner Lua/scanner.l ${CMAKE_CURRENT_BINARY_DIR}/Scanner.cpp)
add_flex_bison_dependency(Scanner Parser)

include_directories(${PROJECT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

set (SRC_FILES
	Generator/Builtins.cpp
	Generator/Generator.cpp
	Generator/Program.cpp
	Generator/Runtime.cpp
	Generator/RValue.cpp
	Generator/Scope.cpp
	Generator/Table.cpp
	Generator/Variable.cpp

	Util/PrettyPrint.cpp

	main.cpp
)

add_executable(theJitter ${FLEX_Scanner_OUTPUTS} ${BISON_Parser_OUTPUTS} ${SRC_FILES})
target_link_libraries(theJitter -lgccjit)
