CC = gcc           # Compilador
CFLAGS = -Wall -g  # Flags de compilação

# Alvo principal: compilar o servidor
all: server

# Regra para compilar o servidor
server: src/server.c
	$(CC) $(CFLAGS) -o server src/server.c

# Limpar os ficheiros binários gerados
clean:
	rm -f server
