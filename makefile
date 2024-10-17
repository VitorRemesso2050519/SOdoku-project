# Definindo variáveis
CC = gcc                # Compilador
CFLAGS = -Wall -g      # Flags de compilação
SRC_DIR = src          # Diretório de código fonte
BIN_DIR = bin          # Diretório para executáveis

# Criando o diretório de binários se não existir
$(shell mkdir -p $(BIN_DIR))

# Nomes dos executáveis
EXEC_SERVER = $(BIN_DIR)/server
EXEC_SERVIDOR = $(BIN_DIR)/servidor

# Regra padrão para compilar todos os executáveis
all: $(EXEC_SERVER) $(EXEC_SERVIDOR)

# Regra para compilar o executável do servidor
$(EXEC_SERVER): $(SRC_DIR)/server.c
	$(CC) $(CFLAGS) -o $@ $<

# Regra para compilar o executável do servidor
$(EXEC_SERVIDOR): $(SRC_DIR)/servidor.c
	$(CC) $(CFLAGS) -o $@ $<

# Limpar os ficheiros binários gerados
clean:
	rm -f $(EXEC_SERVER) $(EXEC_SERVIDOR)

# Regra padrão
.PHONY: all clean
