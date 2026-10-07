CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
SRC = src/main.c src/alocador.c src/estrategias.c src/estatisticas.c
OBJ = $(SRC:.c=.o)
EXEC = alocador

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	rm -f $(OBJ) $(EXEC)
