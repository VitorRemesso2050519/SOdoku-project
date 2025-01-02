# SOdoku-project
Projeto de SO de sudoku.

## Breve esclarecimento de arquitetura:
- config contém os ficheiros de configuração de servidor e dos vários clientes
- data contém os ficheiros de dados do servidor, como os jogos e os recordes
- logs contém os ficheiros de logs dos clientes, do servidor e um ficheiro log comum para a simulação de vários clientes
- src contém o código

## Para correr este projeto:
1. Após ter feito o download para o seu terminal preferido (PuTTY por exemplo), corra o makefile disponível com o comando "make".
2. Aquando a finalização da compilação, abra um terminal extra.
3. Na raiz do projeto terá dois executáveis: client e server. Um terminal será para o Servidor e outro para o Cliente.
4. Num dos terminais, escreva "./server config/server.config". Isto irá inicializar o Servidor.
5. COM O SERVIDOR A CORRER, no outro terminal escreva o seguinte: ./client config/[ficheiro] [n] [modo]
- ficheiro sendo a configuração de cliente que escolheu, n sendo o número de clientes e modo sendo o modo de jogo (singleplayer, multiplayer, incrementaltest).
- OBRIGATORIAMENTE terá que escrever o primeiro argumento (ficheiro de configuração). Se quiser simular vários clientes, por favor especifique o número de clientes e o modo juntamente com o ficheiro de configuração na ordem acima.
- singleplayer gera múltiplos clientes para jogar um jogo qualquer singleplayer, multiplayer gera múltiplos clientes para entrar na sala multiplayer e jogarem tal jogo multiplayer e incrementaltest é um caso multiplayer especial para podermos ver qual tipo de incrementação é o mais rápido.

- NOTA: Se quiser testar incrementaltest, por favor certifique-se que a variavel ROOM_SIZE no ficheiro de configuração do servidor é 81.

## Formatos dos config files:

### Servidor
- PATH_JOGOS: (data/jogos.txt)
- PATH_LOGS: (logs/server.log)
- PATH_STATS: (data/jogos_stats.txt)
- MAX_CLIENTS: (int, maior que 0)
- ROOM_SIZE: (int, de preferência entre 0 e MAX_CLIENTS)

### Cliente
- ID_CLIENTE: (int, de 0 para cima)
- IP_SERVIDOR: (127.0.0.1)
- PATH_LOGS: (logs/client.log)
- IS_VIP: (bool (0 ou 1))
- PARTIAL_NUM: (int, entre 1 e 81)

