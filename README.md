# SOdoku-project
Projeto de SO de sudoku.

Para fazer (1ª etapa):

- Bibliotecas para gestão da informação nos ficheiros de texto.
- Servidor e Client têm que carregar paramêtros para config file
- Tbm devem exportar pra 1 (ou +) o registo dos eventos que ocorrem nas apps.
- Função de verificação de solução:
- AKA recebe uma solução e verifica se está correta, registando o resultado: correta, errada em x números.

Para testar o servidor (1ªfase):
- Criar makefile na raiz, com o seguinte código:
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