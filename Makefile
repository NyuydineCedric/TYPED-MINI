CC = gcc
BISON = bison
FLEX = flex

CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -g
LDLIBS = -lm

TARGET = typed-mini.exe

SOURCES = \
	src/ast/ast.c \
	src/semantic/symbol_table.c \
	src/semantic/type_checker.c \
	src/ir/three_address_code.c \
	src/optimiser/optimizer.c \
	src/codegen/codegen.c \
	src/driver/main.c \
	src/parser/parser.tab.c \
	src/lexer/lex.yy.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDLIBS)

src/parser/parser.tab.c src/parser/parser.tab.h &: src/parser/parser.y
	$(BISON) -Wall -d -o src/parser/parser.tab.c src/parser/parser.y

src/lexer/lex.yy.c: src/lexer/lexer.l src/parser/parser.tab.h
	$(FLEX) -o src/lexer/lex.yy.c src/lexer/lexer.l

test: $(TARGET)
	bash run_tests.sh

clean:
	rm -f $(TARGET)

.PHONY: all test clean