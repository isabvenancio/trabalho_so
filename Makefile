CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
SRC = src/main.c src/alocador.c
OBJ = $(SRC:.c=.o)
EXEC = alocador

all: $(EXEC)

\((EXEC):\)(OBJ)
	\((CC)\)(CFLAGS) -o \(@\)^

clean:
	rm -f \((OBJ)\)(EXEC)