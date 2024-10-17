CC = gcc                # Compilador
CFLAGS = -Wall -g      # Flags de compilação

# Alvo principal: compilar o servidor e cliente
all: server client

# Regra para compilar o servidor
server: src/server.c src/config.c
	$(CC) $(CFLAGS) -o server src/server.c src/config.c

# Regra para compilar o cliente
client: src/client.c src/config.c
	$(CC) $(CFLAGS) -o client src/client.c src/config.c

# Limpar os ficheiros binários gerados
clean:
	rm -f server client
