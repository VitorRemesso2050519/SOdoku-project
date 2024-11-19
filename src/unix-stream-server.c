#include "unix.h"
#include "util-stream-server.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>      // For strptime
#include <semaphore.h> // For sem_init, sem_post
#include <pthread.h>   // For pthread_create, pthread_detach

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

ConfigServidor config;
sem_t client_semaphore;

int num_jogos = 0; // Define num_jogos
Jogo jogos[100];   // Define jogos

void handle_client(int client_socket, int num_jogos, Jogo jogos[]) {
    char buffer[BUFFER_SIZE];
    int action_code, client_id;
    int game_id, n_posicoes;
    int posicoes[BUFFER_SIZE]; // Tamanho máximo possível
    char numeros[BUFFER_SIZE]; // Tamanho máximo possível
    int error_positions[BUFFER_SIZE]; // Tamanho máximo possível
    Jogo *game;
    int errors;

    while (1) {
        // Receive the client's message as a string
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        sscanf(buffer, "%d %d", &action_code, &client_id);
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
            sem_post(&client_semaphore);
            return;
        }

        switch (action_code) {
            case CODE_REQUEST_NEW_GAME:
                log_event(config.log_file, client_id, CODE_REQUEST_NEW_GAME, "Client requested a new game.");

                // Fetch a random game from the available list
                Jogo new_game = grabRandomGame(jogos, num_jogos);  // Helper method to fetch a new game

                snprintf(buffer, BUFFER_SIZE, "%d %d %d %s", CODE_RESPONSE_NEW_GAME, client_id, new_game.id_jogo, new_game.tabuleiro);

                log_event(config.log_file, client_id, CODE_RESPONSE_NEW_GAME, "Server responded with a new game.");
                
                send(client_socket, buffer, strlen(buffer), 0);
                break;
            case CODE_SEND_PARTIAL_SOLUTION:
                sscanf(buffer + 4, "%d %d", &game_id, &n_posicoes);
                sscanf(buffer + 4 + sizeof(int) * 2, "%s %s", (char*)posicoes, numeros);
                game = &jogos[game_id];
                // Validate partial solution
                bool partial_correct = true;
                errors = 0;
                for (int i = 0; i < n_posicoes; i++) {
                    if (!verificarPosicao(game->tabuleiro, posicoes[i], game->solucao)) {
                        partial_correct = false;
                        error_positions[errors] = posicoes[i];
                        errors++;
                    }
                }
                if (partial_correct) {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d", CODE_RESPONSE_CORRECT_PARTIAL, client_id, 0);
                    log_event(config.log_file, client_id, CODE_RESPONSE_CORRECT_PARTIAL, "Partial solution is correct.");
                } else {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d ", CODE_RESPONSE_INCORRECT_PARTIAL, client_id, errors);
                    for (int i = 0; i < errors; i++) {
                        char pos_str[4];
                        snprintf(pos_str, sizeof(pos_str), "%d ", error_positions[i]);
                        strncat(buffer, pos_str, BUFFER_SIZE - strlen(buffer) - 1);
                    }
                    log_event(config.log_file, client_id, CODE_RESPONSE_INCORRECT_PARTIAL, "Partial solution is incorrect.");
                }
                send(client_socket, buffer, strlen(buffer), 0);
                break;
            case CODE_SEND_FINAL_SOLUTION: {
                log_event(config.log_file, client_id, CODE_SEND_FINAL_SOLUTION, "Client submitted the final solution.");

                // Extract the solution sent by the client
                char client_solution[81];
                sscanf(buffer + 4, "%d %s", &game_id, client_solution); 

                // Validate the client’s solution against the correct solution
                game = &jogos[game_id];
                errors = verificarJogoCompleto(client_solution, game->solucao);

                // Prepare a response based on the solution check
                if (errors == 0) {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d", CODE_RESPONSE_CORRECT_FINAL, client_id, 0);
                    log_event(config.log_file, client_id, CODE_RESPONSE_CORRECT_FINAL, "Final client solution is correct.");

                    // Update game statistics
                    JogoState jogoState;
                    jogoState.id_jogo = game_id;
                    jogoState.attempts = 1; // Update this with the actual number of attempts
                    jogoState.record_time = time(NULL); // Update this with the actual record time

                    JogoState existingState;
                    if (lerEstatisticasJogo("data/jogos_stats.txt", game_id, &existingState)) {
                        if (difftime(jogoState.record_time, existingState.record_time) < 0 ||
                            (difftime(jogoState.record_time, existingState.record_time) == 0 && jogoState.attempts < existingState.attempts)) {
                            if (escreverEstatisticasJogo("data/jogos_stats.txt", &jogoState)) {
                                log_event(config.log_file, client_id, CODE_NEW_RECORD, "Game statistics updated successfully.");
                            } else {
                                log_event(config.log_file, client_id, CODE_RESPONSE_ERROR, "Failed to update game statistics.");
                            }
                        } else {
                            log_event(config.log_file, client_id, CODE_NOT_RECORD, "New statistics are not better than existing ones.");
                        }
                    }
                } else {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d", CODE_RESPONSE_INCORRECT_FINAL, client_id, errors);
                    char log_message[BUFFER_SIZE];
                    snprintf(log_message, BUFFER_SIZE, "Final client solution has %d errors.", errors);
                    log_event(config.log_file, client_id, CODE_RESPONSE_INCORRECT_FINAL, log_message);
                }
                if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                    perror("send");
                }
                break;
            }
            case CODE_REQUEST_STATS: {
                log_event(config.log_file, client_id, CODE_REQUEST_STATS, "Client requested game statistics.");
                JogoState jogoState;
                if (lerEstatisticasJogo("data/jogos_stats.txt", game_id, &jogoState)) {
                    char record_time_str[9];
                    strftime(record_time_str, sizeof(record_time_str), "%H:%M:%S", localtime(&jogoState.record_time));
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d %s %d", CODE_RESPONSE_STATS, client_id, jogoState.id_jogo, record_time_str, jogoState.attempts);
                    log_event(config.log_file, client_id, CODE_RESPONSE_STATS, "Server responded with game statistics.");
                } else {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d", CODE_RESPONSE_STATS, client_id, -1);
                    log_event(config.log_file, client_id, CODE_RESPONSE_STATS, "Game statistics not found.");
                }
                if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                    perror("send");
                }
                break;
            }
            //Multiplayer Commands will be introduced later
            default: {
                log_event(config.log_file, client_id, CODE_RESPONSE_INVALID_COMMAND, "Client sent an invalid command.");
                snprintf(buffer, BUFFER_SIZE, "%d %d", CODE_RESPONSE_INVALID_COMMAND, client_id);
                if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                    perror("send");
                }
                break;
            }
        }

    }

}

void* client_thread(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int action_code;
    int client_id;

    // Receive the initial message from the client
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        perror("Failed to receive initial message from client");
        close(client_socket);
        sem_post(&client_semaphore);
        return NULL;
    }
    buffer[bytes_received] = '\0';
    sscanf(buffer, "%d %d", &action_code, &client_id);

    if (action_code != CODE_NEW_CLIENT) {
        printf("Invalid initial message from client\n");
        close(client_socket);
        sem_post(&client_semaphore);
        return NULL;
    }

    log_event(config.log_file, client_id, CODE_NEW_CLIENT, "New client connected.");

    // Handle the client
    handle_client(client_socket, num_jogos, jogos);

    return NULL;
}
int main(int argc, char* argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_id; // Declare client_id here

    if (argc < 2) {
        printf("Uso: %s <ficheiro_configuracao>\n", argv[0]);
        return 1;
    }

    // Ler a configuração do servidor
    lerConfiguracaoServidor(argv[1], &config);

    // Carregar os jogos a partir do ficheiro de jogos especificado na configuração
    carregarJogos(config.path_jogos, jogos, &num_jogos);

    // Create a UNIX domain socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set up the socket address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    server_addr.sin_port = htons(8080);

    // Bind the socket to the specified path
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to bind socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Failed to listen on socket");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    sem_init(&client_semaphore, 0, MAX_CLIENTS);

    // Main server loop
    while (1) {
        // Initialize addr_len before accepting a new client connection
        addr_len = sizeof(client_addr);

        // Accept a new client connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket == -1) {
            perror("Failed to accept client connection");
            continue;
        }

        // Create a thread to handle the client
        pthread_t thread_id;
        int* new_sock = malloc(sizeof(int));
        if (new_sock == NULL) {
            perror("Failed to allocate memory for new socket");
            close(client_socket);
            continue;
        }
        *new_sock = client_socket;
        if (pthread_create(&thread_id, NULL, client_thread, (void*)new_sock) != 0) {
            perror("Failed to create thread");
            free(new_sock);
            close(client_socket);
            continue;
        }

        // Detach the thread to avoid resource leaks
        pthread_detach(thread_id);
    }

    // Clean up the semaphore
    sem_destroy(&client_semaphore);

    // Close the server socket
    close(server_socket);

    return 0;
}
