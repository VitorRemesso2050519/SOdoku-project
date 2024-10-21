# Definições
CC = gcc                  # Compilador
CFLAGS = -Wall -g -I$(SRC_DIR)  # Flags de compilação (inclui o diretório src para cabeçalhos)

# Executáveis
EXECUTABLE_SERVER = server
EXECUTABLE_CLIENT = client

# Diretórios
SRC_DIR = src
LOG_DIR = logs

# Fontes
SERVER_SRC = $(SRC_DIR)/server.c $(SRC_DIR)/utils.c
CLIENT_SRC = $(SRC_DIR)/client.c $(SRC_DIR)/utils.c

# Cabeçalhos
HEADER = $(SRC_DIR)/utils.h

# Objetos
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# Alvo principal: compilar os executáveis
all: $(EXECUTABLE_SERVER) $(EXECUTABLE_CLIENT)

# Regra para compilar o servidor
$(EXECUTABLE_SERVER): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $(EXECUTABLE_SERVER) $(SERVER_OBJ)

# Regra para compilar o cliente
$(EXECUTABLE_CLIENT): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $(EXECUTABLE_CLIENT) $(CLIENT_OBJ)

# Regra genérica para compilar arquivos .o, considerando a dependência do header
%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

# Limpar os ficheiros binários gerados
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(EXECUTABLE_SERVER) $(EXECUTABLE_CLIENT)

.PHONY: all clean run create_logs