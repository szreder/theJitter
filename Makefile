.PHONY : clean distclean

BIN = theJitter
OBJS = grammar.tab.o lex.yy.o main.o Generator/Builtins.o Generator/Generator.o Generator/Program.o Generator/Runtime.o Generator/RValue.o Generator/Variable.o Util/PrettyPrint.o
CXXFLAGS = -std=c++17 -I . -ggdb
CFLAGS = $(CXXFLAGS)
CC = $(CXX)
LDFLAGS = -lgccjit

$(BIN) : $(OBJS)
	g++ $(LDFLAGS) $(OBJS) -o $@

grammar.tab.c : Lua/grammar.y
	bison -d Lua/grammar.y

lex.yy.c : Lua/scanner.l
	flex Lua/scanner.l

clean :
	rm -f $(OBJS) grammar.tab.{c,h} lex.yy.c

distclean :
	make clean
	rm -f $(BIN)
