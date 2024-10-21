# Definições
CC = gcc                  # Compilador
CFLAGS = -Wall -g        # Flags de compilação

# Executáveis
EXECUTABLE_SERVER = server
EXECUTABLE_CLIENT = client

# Diretórios
SRC_DIR = src
LOG_DIR = logs
CONFIG_DIR = config

# Fontes
SERVER_SRC = $(SRC_DIR)/server.c $(SRC_DIR)/utils.c
CLIENT_SRC = $(SRC_DIR)/client.c $(SRC_DIR)/utils.c

# Cabeçalhos
HEADER = $(SRC_DIR)/utils.h

# Alvo principal: compilar os executáveis
all: $(EXECUTABLE_SERVER) $(EXECUTABLE_CLIENT)

# Regra para compilar o servidor
$(EXECUTABLE_SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(EXECUTABLE_SERVER) $(SERVER_SRC)

# Regra para compilar o cliente
$(EXECUTABLE_CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(EXECUTABLE_CLIENT) $(CLIENT_SRC)

# Regra genérica para compilar arquivos .o
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Limpar os ficheiros binários gerados
clean:
	rm -f $(EXECUTABLE_SERVER) $(EXECUTABLE_CLIENT)

# Criar logs (caso você tenha um script de log para gerar)
create_logs:
	touch $(LOG_DIR)/client.log $(LOG_DIR)/server.log

# Executar
run: all create_logs
	./$(EXECUTABLE_SERVER) $(CONFIG_DIR)/server.config &
	./$(EXECUTABLE_CLIENT) $(CONFIG_DIR)/client.config

.PHONY: all clean run create_logs