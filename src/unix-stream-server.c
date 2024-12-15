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

ConfigServidor config;
sem_t client_semaphore;
pthread_mutex_t log_mutex, record_mutex;

Jogo jogos[100];
int num_jogos = 0;
int current_client_ammount = 0;

void* client_thread(void* arg) {
    int client_socket = *(int*)arg, client_id, message_code, game_id, n_posicoes, errors, attempts;
    char buffer[BUFFER_SIZE], tabuleiro[81], numeros[81], posicoes[81], error_positions[81];
    double record_time;
    Jogo game;
    ssize_t bytes_received;

    while (1) {
        bool partial_correct = true;
        // Receive message
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received == -1) {
            perror("Failed to receive message from client");
            close(client_socket);
            return NULL;
        }
        buffer[bytes_received] = '\0'; // Ensure null-termination
        printf("Received message: %s\n", buffer); // Debug print

        // Parse the first two variables in the message
        sscanf(buffer, "%d %d", &client_id, &message_code);
        printf("Parsed client_id: %d, message_code: %d\n", client_id, message_code); // Debug print


        // Process the message based on the message code
        switch (message_code) {
            case CODE_NEW_CLIENT: //DONE
                printf("Client %d connected.\n", client_id);
                pthread_mutex_lock(&log_mutex);
                log_event(config.log_file, client_id, CODE_NEW_CLIENT, "Client connected.");
                pthread_mutex_unlock(&log_mutex);
                current_client_ammount++;
                printf("Current client ammount: %d\n", current_client_ammount);
                memset(buffer, 0, BUFFER_SIZE);
                break;
            case CODE_REQUEST_NEW_GAME: { //DONE
                // Handle new game request
                game = grabRandomGame(jogos, num_jogos);
                pthread_mutex_lock(&log_mutex);
                log_event(config.log_file, client_id, CODE_REQUEST_NEW_GAME, "Client is asking for new game.");
                pthread_mutex_unlock(&log_mutex);
                printf("Client %d requested a new game, sending game %d: %s\n", client_id, game.id_jogo, game.tabuleiro);
                snprintf(buffer, BUFFER_SIZE, "%d %d %d %s", client_id, CODE_RESPONSE_NEW_GAME, game.id_jogo, game.tabuleiro);
                if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                    perror("Send new game");
                }
                pthread_mutex_lock(&log_mutex);
                log_event(config.log_file, client_id, CODE_RESPONSE_NEW_GAME, "New game sent to client.");
                pthread_mutex_unlock(&log_mutex);
                memset(buffer, 0, BUFFER_SIZE);
                break;
            }
            case CODE_SEND_PARTIAL_SOLUTION:
                sscanf(buffer, "%d %d %d %d", &client_id, &message_code, &game_id, &n_posicoes);
                printf("Parsed values: client_id=%d, message_code=%d, game_id=%d, n_posicoes=%d\n", client_id, message_code, game_id, n_posicoes);

                // Validate game_id
                if (game_id < 0 || game_id > num_jogos) {
                    printf("Invalid game_id: %d\n", game_id);
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d", client_id, CODE_RESPONSE_ERROR, game_id);
                    send(client_socket, buffer, strlen(buffer), 0);
                    memset(buffer, 0, BUFFER_SIZE);
                    break;
                }

                game = jogos[game_id-1];
                printf("Game solution: %s\n", game.solucao); // Debug print

                // Use n_positions to read the positions and numbers from the buffer
                int offset = 0;
                for (int i = 0; i < 4; i++) {
                    while (buffer[offset] != ' ') offset++;
                    offset++;
                }

                for (int i = 0; i < n_posicoes; i++) {
                    int pos;
                    int num;
                    sscanf(buffer + offset, "%d %d", &pos, &num);
                    posicoes[i] = pos;
                    numeros[i] = num + '0'; // Convert integer to character
                    printf("Parsed position and number: posicoes[%d]=%d, numeros[%d]=%c\n", i, posicoes[i], i, numeros[i]);
                    while (buffer[offset] != ' ' && buffer[offset] != '\0') offset++;
                    offset++;
                    while (buffer[offset] != ' ' && buffer[offset] != '\0') offset++;
                    offset++;
                }

                // Validate partial solution
                errors = 0;
                for (int i = 0; i < n_posicoes; i++) {
                    printf("Checking position %d with number %c\n", posicoes[i], numeros[i]);
                    if (!verificarPosicao(numeros[i], posicoes[i], game.solucao)) {
                        partial_correct = false;
                        error_positions[errors] = posicoes[i];
                        errors++;
                    }
                }
                if (partial_correct) {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d", client_id, CODE_RESPONSE_CORRECT_PARTIAL, 0);
                    pthread_mutex_lock(&log_mutex);
                    log_event(config.log_file, client_id, CODE_RESPONSE_CORRECT_PARTIAL, "Partial solution is correct.");
                    pthread_mutex_unlock(&log_mutex);
                } else {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d ", client_id, CODE_RESPONSE_INCORRECT_PARTIAL, errors);
                    for (int i = 0; i < errors; i++) {
                        char pos_str[4];
                        snprintf(pos_str, sizeof(pos_str), "%d ", error_positions[i]);
                        strncat(buffer, pos_str, BUFFER_SIZE - strlen(buffer) - 1);
                    }
                    pthread_mutex_lock(&log_mutex);
                    log_event(config.log_file, client_id, CODE_RESPONSE_INCORRECT_PARTIAL, "Partial solution is incorrect.");
                    pthread_mutex_unlock(&log_mutex);
                }
                printf("Sending response: %s\n", buffer); // Debug print
                send(client_socket, buffer, strlen(buffer), 0);
                memset(buffer, 0, BUFFER_SIZE);
                break;
            case CODE_SEND_FINAL_SOLUTION: {
                pthread_mutex_lock(&log_mutex);
                log_event(config.log_file, client_id, CODE_SEND_FINAL_SOLUTION, "Client submitted the final solution.");
                pthread_mutex_unlock(&log_mutex);

                // Extract the solution sent by the client

                sscanf(buffer, "%d %d %d %d %.2f %81s", &client_id, &message_code, &game_id, &attempts, &record_time, tabuleiro); 

                // Validate the client’s solution against the correct solution
                game = jogos[game_id-1];
                errors = verificarJogoCompleto(tabuleiro, game.solucao);

                // Prepare a response based on the solution check
                if (errors == 0) {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d", CODE_RESPONSE_CORRECT_FINAL, client_id);
                    send(client_socket, buffer, strlen(buffer), 0);
                    pthread_mutex_lock(&log_mutex);
                    log_event(config.log_file, client_id, CODE_RESPONSE_CORRECT_FINAL, "Final client solution is correct.");
                    pthread_mutex_unlock(&log_mutex);

                    // Update game statistics
                    JogoState jogoState;
                    jogoState.id_jogo = game_id;
                    jogoState.attempts = attempts; // Update this with the actual number of attempts
                    jogoState.record_time = record_time; // Update this with the actual record time

                    JogoState existingState;
                    pthread_mutex_lock(&record_mutex);
                    if (lerEstatisticasJogo("data/jogos_stats.txt", game_id, &existingState)) {
                        if (difftime(jogoState.record_time, existingState.record_time) < 0 ||
                            (difftime(jogoState.record_time, existingState.record_time) == 0 && jogoState.attempts < existingState.attempts)) {
                            if (escreverEstatisticasJogo("data/jogos_stats.txt", &jogoState)) {
                                pthread_mutex_lock(&log_mutex);
                                log_event(config.log_file, client_id, CODE_NEW_RECORD, "Game statistics updated successfully. New record!");
                                snprintf(buffer, BUFFER_SIZE, "%d %d %d", CODE_NEW_RECORD, client_id);
                                send(client_socket, buffer, strlen(buffer), 0);
                                pthread_mutex_unlock(&log_mutex);
                            } else {
                                pthread_mutex_lock(&log_mutex);
                                log_event(config.log_file, client_id, CODE_RESPONSE_ERROR, "Failed to update game statistics.");
                                pthread_mutex_unlock(&log_mutex);
                            }
                        } else {
                            pthread_mutex_lock(&log_mutex);
                            log_event(config.log_file, client_id, CODE_NOT_RECORD, "New statistics are not better than existing ones.");
                            pthread_mutex_unlock(&log_mutex);
                            snprintf(buffer, BUFFER_SIZE, "%d %d %d", CODE_NOT_RECORD, client_id);
                            send(client_socket, buffer, strlen(buffer), 0);
                        }
                    }
                    pthread_mutex_unlock(&record_mutex);
                } else {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d", CODE_RESPONSE_INCORRECT_FINAL, client_id, errors);
                    char log_message[BUFFER_SIZE];
                    snprintf(log_message, BUFFER_SIZE, "Final client solution had %d errors.", errors);
                    pthread_mutex_lock(&log_mutex);
                    log_event(config.log_file, client_id, CODE_RESPONSE_INCORRECT_FINAL, log_message);
                    pthread_mutex_unlock(&log_mutex);
                }
                if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                    perror("Send final solution");
                }
                memset(buffer, 0, BUFFER_SIZE);
                break;
            }
            case CODE_REQUEST_STATS: { //DONE
                // Handle game state request
                pthread_mutex_lock(&log_mutex);
                log_event(config.log_file, client_id, CODE_REQUEST_STATS, "Client requested game statistics.");
                pthread_mutex_unlock(&log_mutex);
                JogoState jogoState;
                if (lerEstatisticasJogo("data/jogos_stats.txt", game_id, &jogoState)) {
                    char record_time_str[9];
                    strftime(record_time_str, sizeof(record_time_str), "%H:%M:%S", localtime(&jogoState.record_time));
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d %s %d", client_id, CODE_RESPONSE_STATS, jogoState.id_jogo, record_time_str, jogoState.attempts);
                    pthread_mutex_lock(&log_mutex);
                    log_event(config.log_file, client_id, CODE_RESPONSE_STATS, "Server responded with game statistics.");
                    pthread_mutex_unlock(&log_mutex);
                } else {
                    snprintf(buffer, BUFFER_SIZE, "%d %d %d", client_id, CODE_RESPONSE_STATS, -1);
                    pthread_mutex_lock(&log_mutex);
                    log_event(config.log_file, client_id, CODE_RESPONSE_ERROR, "Game statistics not found.");
                    pthread_mutex_unlock(&log_mutex);
                }
                if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                    perror("Send game statistics");
                }
                memset(buffer, 0, BUFFER_SIZE);
                break;
            }
            case CODE_DISCONNECT: //DONE
                // Handle client disconnection
                printf("Client %d disconnected.\n", client_id);
                pthread_mutex_lock(&log_mutex);
                log_event(config.log_file, client_id, CODE_DISCONNECT, "Client disconnected.");
                pthread_mutex_unlock(&log_mutex);
                current_client_ammount--;
                printf("Current client ammount: %d\n", current_client_ammount);
                memset(buffer, 0, BUFFER_SIZE);
                close(client_socket);
                return NULL;
            default: //DONE
                printf("Unknown message code %d from client %d.\n", message_code, client_id);
                pthread_mutex_lock(&log_mutex);
                log_event(config.log_file, client_id, CODE_RESPONSE_ERROR, "Unknown message code.");
                pthread_mutex_unlock(&log_mutex);
                memset(buffer, 0, BUFFER_SIZE);
                break;
        }
    }

    close(client_socket);
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

    for (int i = 0; i < num_jogos; i++) {
        printf("Game %d: %s\n", jogos[i].id_jogo, jogos[i].tabuleiro);
    }

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

    sem_init(&client_semaphore, 0, config.max_clients);

    // Inicializar o mutex para os logs
    if (pthread_mutex_init(&log_mutex, NULL) != 0) {
        perror("Failed to initialize log mutex");
        return 1;
    }

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

    // Destroy the mutex for logs
    pthread_mutex_destroy(&log_mutex);

    // Close the server socket
    close(server_socket);

    return 0;
}

// sincronização nos logs (pode haver diferentes clientes a fazer pedidos ao mesmo tempo, o que o server tem que registar)