# SOdoku-project
Projeto de SO de sudoku.

## Para correr este projeto:
1. Após ter feito o download e posto no seu terminal preferido (PuTTY por exemplo), corra o makefile disponível.
2. Com duas janelas abertas diferentes. Uma será para o Servidor e outra para o Cliente.
3. Numa das janelas, escreva "./server config/server.config". Isto irá inicializar o Servidor.
4. COM O SERVIDOR A CORRER, na outra janela escreva "./client config/client.config". Isto irá inicializar o Cliente.
- Poderá correr outros ficheiros configs, mas use o mesmo formato disponível a seguir.
5. Se quiser criar vários clientes ao mesmo tempo, pode especifique quantos e o modo de jogo que irão jogar.
- Para tal, escreva ./client config/client.config [n] [modo]
- n sendo o número de clientes e modo sendo o modo de jogo (singleplayer, multiplayer, incrementaltest).
- incrementaltest é um caso multiplayer especial para podermos ver qual tipo de incrementação é o mais rápido.

## Formatos dos config files:

### Servidor
- PATH_JOGOS: (data/jogos.txt)
- PATH_LOGS: (logs/server.log)
- PATH_STATS: (data/jogos_stats.txt)
- MAX_CLIENTS: (int, de 0 para cima)
- ROOM_SIZE: (int, de preferência entre 0 e MAX_CLIENTS)

### Cliente
- ID_CLIENTE: (int, de 0 para cima)
- IP_SERVIDOR: (127.0.0.1)
- PATH_LOGS: (logs/client.log)
- IS_VIP: (bool (0 ou 1))
- PARTIAL_NUM: (int, entre 0 e 81)

