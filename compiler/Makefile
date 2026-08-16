CC = gcc
CFLAGS = -O2 -std=c11
SRCDIR = src
TARGET = simply-compiler

SRCS = $(SRCDIR)/main.c \
       $(SRCDIR)/lexer.c \
       $(SRCDIR)/parser.c \
       $(SRCDIR)/ast.c \
       $(SRCDIR)/semantic.c \
       $(SRCDIR)/symbol_table.c \
       $(SRCDIR)/tac.c \
       $(SRCDIR)/optimizer.c \
       $(SRCDIR)/codegen.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lm

clean:
	rm -f $(TARGET) $(TARGET).exe

.PHONY: all clean
