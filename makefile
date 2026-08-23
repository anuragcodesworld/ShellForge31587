CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

SRC = src/expand.c \
      src/lexer.c \
      src/main.c \
      src/parser.c \
      src/token.c \
      src/builtin.c \
      src/executor.c

all:
	$(CC) $(CFLAGS) $(SRC) -lreadline -o shellforge

clean:
	rm -f shellforge
