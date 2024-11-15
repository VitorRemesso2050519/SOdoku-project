#include "unix.h"
#include "util-stream-server.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUFFER_SIZE 512
#define MAX_CLIENTS 10

ConfigServidor config;

void handle_client(int client_socket, int num_jogos, Jogo jogos[]) {
    char buffer[BUFFER_SIZE];

    while (1) {
        // Receive the client's message as a string
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            // Client has disconnected or there was an error
            if (bytes_received == 0) {
                // Client disconnected gracefully
                log_event(config.log_file, client_id, CODE_DISCONNECT, "Client disconnected.");
            } else {
                // An error occurred
                perror("recv failed");
            }
            close(client_socket);  // Clean up the socket
            return;
        }

        int action_code, client_id;
        sscanf(buffer, "%d %d", &action_code, &client_id);  // Example: "1 123 ..." means action code 1, client_id 123

        switch (action_code) {
            case CODE_REQUEST_NEW_GAME:
                log_event(config.log_file, client_id, CODE_REQUEST_NEW_GAME, "Client requested a new game.");

                // Fetch a random game from the available list
                Jogo new_game = grabRandomGame(jogos, num_jogos);  // Helper method to fetch a new game

                snprintf(buffer, BUFFER_SIZE, "%d %d %c", CODE_RESPONSE_NEW_GAME, client_id, new_game.tabuleiro);

                log_event(config.log_file, request.id_cliente, CODE_RESPONSE_NEW_GAME, "Server responded with a new game.");
                break;
            case CODE_SEND_PARTIAL_SOLUTION:
                log_event(config.log_file, client_id, CODE_SEND_PARTIAL_SOLUTION, "Client submitted a partial solution.");

                int game_id;
                int n_posicoes;
                int posicoes[n_posicoes];
                int numeros[n_posicoes];
                sscanf(buffer + 4, "%d %d %d %d", game_id, n_posicoes, posicoes[n_posicoes+1], numeros[n_posicoes+1]);

                Jogo *game = &jogos[game_id];
                //Continuar aqui.

            case CODE_SEND_FINAL_SOLUTION:
                log_event(config.log_file, client_id, CODE_SEND_FINAL_SOLUTION, "Client submitted the final solution.");

                // Extract the solution sent by the client
                char client_solution[81];
                int game_id;
                sscanf(buffer + 4, "%d %s", game_id, client_solution); 

                // Validate the client’s solution against the correct solution
                Jogo *game = &jogos[game_id];
                int errors = verificarJogoCompleto(client_solution, game->solucao);

                // Prepare a response based on the solution check
                if (errors == 0) {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d", CODE_RESPONSE_CORRECT_FINAL, client_id, 0);
                    log_event(config.log_file, client_id, CODE_RESPONSE_CORRECT_FINAL, "Final client solution is correct.");
                } else {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d", CODE_RESPONSE_INCORRECT_FINAL, client_id, errors);
                    log_event(config.log_file, client_id, CODE_RESPONSE_INCORRECT_FINAL, "Final client solution has " + errors + " errors.");
                }
                break;
            //Multiplayer Commands will be introduced later
            default:
                log_event(config.log_file, request.id_cliente, request.code, "Client sent an invalid command.");
                response.code = CODE_RESPONSE_INVALID_COMMAND;
                og_event(config.log_file, request.id_cliente, CODE_RESPONSE_INVALID_COMMAND, "Server responded with an invalid command.");
                break;
        }

        send(client_socket, buffer, strlen(buffer), 0);
    }

}

void *client_thread(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    handle_client(client_socket, num_jogos, jogos); // Pass additional needed arguments

    return NULL;
}

int main(int argc, char* argv[]) {
    int server_socket, client_socket;
    struct sockaddr_un server_addr;

    if (argc < 2) {
        printf("Uso: %s <ficheiro_configuracao>\n", argv[0]);
        return 1;
    }

    Jogo jogos[100];   // Suporte para até 100 jogos por simplicidade´
    int num_jogos = 0;

    // Ler a configuração do servidor
    lerConfiguracaoServidor(argv[1], &config);

    // Carregar os jogos a partir do ficheiro de jogos especificado na configuração
    carregarJogos(config.path_jogos, jogos, &num_jogos);

    // Create a UNIX domain socket
    if ((server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set up the socket address structure
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, UNIXSTR_PATH, sizeof(server_addr.sun_path) - 1);

    // Bind the socket to the specified path
    unlink(UNIXSTR_PATH); // Remove any existing file
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Failed to bind socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Failed to listen on socket");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on %s\n", UNIXSTR_PATH);

    // Main server loop
    while (1) {
        // Accept a new client connection
        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (*client_sock_ptr < 0) {
            perror("accept failed");
            free(client_sock_ptr);
            continue;
        }

        // Create a new thread to handle this client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_thread, client_sock_ptr) != 0) {
            perror("pthread_create failed");
            close(*client_sock_ptr);
            free(client_sock_ptr);
            continue;
        }

        // Detach the thread to handle its own resources
        pthread_detach(thread_id);
    }

    // Close the server socket
    close(server_socket);
    unlink(UNIXSTR_PATH); // Clean up the socket file

    return 0;
}
