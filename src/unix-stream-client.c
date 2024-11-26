#include "unix.h"
#include "util-stream-client.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h> // Add this include for time functions

#define BUFFER_SIZE 1024
#define PORT 8080

ConfigCliente config;
int game_in_progress = 0; // Declare the flag

typedef struct {
    int id_jogo;
    char tabuleiro[82]; // 81 characters + null terminator
    time_t hora_inicio; // Declare hora_inicio
} GameData;

typedef struct {
    int client_socket;
    ConfigCliente config;
    GameData game_data;
} ThreadArgs;

void request_new_game(int client_socket, ConfigCliente config);
void request_game_statistics(int client_socket);
void request_client_statistics(int client_socket);
GameData receive_new_game(ConfigCliente config);
void receive_game_statistics(int client_socketNo);
void display_menu();
void display_game_status();
void* solve_game(void* arg);

int main(int argc, char* argv[]) {
    struct sockaddr_in server_addr;
    int client_socket;
    pthread_t solver_thread;

    if (argc < 2) {
        printf("Uso: %s <ficheiro_configuracao>\n", argv[0]);
        return 1;
    }

    // Inicializar a configuração do cliente
    lerConfiguracaoCliente(argv[1], &config);
    log_event(config.log_file, config.id_cliente, 0, "Client configuration loaded.");

    // Create a socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Failed to create socket");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to create socket.");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, config.server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Invalid address/ Address not supported.");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to connect to server");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to connect to server.");
        exit(EXIT_FAILURE);
    }
    printf("Connected to the server.\n");
    log_event(config.log_file, config.id_cliente, CODE_NEW_CLIENT, "Connected to the server.");

    // Send the initial message to the server
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%d %d", CODE_NEW_CLIENT, config.id_cliente);
    if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
        perror("Failed to send initial message to server");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send initial message to server.");
        exit(EXIT_FAILURE);
    }

    // Main loop to handle user commands
    int command;
    while (1) {
        char buffer[BUFFER_SIZE];
        display_menu();
        scanf("%d", &command);
        switch (command) {
            case 1:
                if (game_in_progress) {
                    printf("A game is already in progress. Please wait until it is solved.\n");
                } else {
                    request_new_game(client_socket, config);
                }
                break;
            case 2:
                multiplayer();
                break;
            case 3:
                request_game_statistics(client_socket); // sends request to server asking for info on the game client is currently playing
                break;
            case 4:
                request_client_statistics(client_socket); // grabs info from the config file and shows it to the user
                break;
            case 0:
                snprintf(buffer, BUFFER_SIZE, "%d %d", CODE_DISCONNECT, config.id_cliente);
                if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                    perror("Failed to send disconnect request to server");
                    log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send disconnect request to server.");
                }
                close(client_socket);
                printf("Disconnected from the server.\n");
                log_event(config.log_file, config.id_cliente, CODE_DISCONNECT, "Disconnected from the server.");
                return 0;
            default:
                printf("Invalid command.\n");
                break;
        }
    }
}

void request_new_game(int client_socket, ConfigCliente config) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%d %d", CODE_REQUEST_NEW_GAME, config.id_cliente);
    if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
        perror("Failed to send new game request to server");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send new game request to server.");
    } else {
        printf("New game request sent to the server.\n");
        log_event(config.log_file, config.id_cliente, CODE_REQUEST_NEW_GAME, "New game request sent to the server.");
        GameData game_data = receive_new_game(config);

        game_in_progress = 1;

        pthread_create(&solver_thread, NULL, solve_game, &game_data);
    }
}

void request_game_statistics(int client_socket) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%d %d %d", CODE_REQUEST_STATS, config.id_cliente, id_jogo);
    if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
        perror("Failed to send game statistics request to server");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send game statistics request to server.");
    } else {
        printf("Game statistics request sent to the server.\n");
        log_event(config.log_file, config.id_cliente, CODE_REQUEST_STATS, "Game statistics request sent to the server.");
        receive_game_statistics();
    }
}

GameData receive_new_game(ConfigCliente config) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    GameData game_data = {0, {0}}; // Initialize with default values

    if (bytes_received == -1) {
        perror("Failed to receive new game from server");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive new game from server.");
        return game_data;
    }
    buffer[bytes_received] = '\0';

    int response_code, client_id;
    char new_board[82] = {0}; // Initialize with zeros to ensure null-termination

    int parsed_items = sscanf(buffer, "%d %d %d %81s", &response_code, &client_id, &game_data.id_jogo, new_board);
    if (parsed_items != 4) {
        printf("Failed to parse new game response. Parsed items: %d\n", parsed_items);
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to parse new game response.");
        return game_data;
    }

    if (response_code == CODE_RESPONSE_NEW_GAME) {
        strncpy(game_data.tabuleiro, new_board, 82);
        hora_inicio = time(NULL);
        printf("New game received from server. Game ID: %d\n", game_data.id_jogo);
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_NEW_GAME, "New game received from server.");
    } else {
        printf("Failed to receive new game. Server response code: %d\n", response_code);
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive new game. Invalid response code.");
    }

    game_data.config = config;

    return game_data;
}

void receive_game_statistics(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received == -1) {
        perror("Failed to receive game statistics from server");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive game statistics from server.");
        return;
    }
    buffer[bytes_received] = '\0';

    int response_code, client_id, game_id, attempts;
    char record_time_str[9];
    sscanf(buffer, "%d %d %d %s %d", &response_code, &client_id, &game_id, record_time_str, &attempts);

    if (response_code == CODE_RESPONSE_STATS) {
        printf("Game Statistics:\n");
        printf("Game ID: %d\n", game_id);
        printf("Record Time: %s\n", record_time_str);
        printf("Attempts: %d\n", attempts);
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_STATS, "Game statistics received from server.");
    } else {
        printf("Failed to receive game statistics. Server response code: %d\n", response_code);
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive game statistics. Invalid response code.");
    }
}

void request_client_statistics() {
    printf("\n======== Sudoku Client Statistics =========\n");
    printf("Solved Games: %d\n", config.games_solved);
    printf("Errors Sent: %d\n", config.errors_sent);
    printf("=============================================\n");
}

void display_menu() {
    printf("\n========== Sudoku Client Interface ==========\n");
    printf("1: Request New Game\n");
    printf("2: Multiplayer\n"); //Mudou!
    printf("3: Request Current Game Statistics\n");
    printf("4: View Client Statistics\n");
    printf("0: Disconnect\n");
    printf("=============================================\n");
    printf("Enter command number: ");
}

void* solve_game(void* arg) {
    ThreadArgs* thread_args = (ThreadArgs*)arg;
    int client_socket = thread_args->client_socket;
    GameData* game_data = &thread_args->game_data;

    while (1) {
        bool is_full = true;
        for (int i = 0; i < 81; i++) {
            if (game_data->tabuleiro[i] == '0' || game_data->tabuleiro[i] == 'X') {
                is_full = false;
                break;
            }
        }

        if (is_full) {
            // Send the full solution to the server
            char message[BUFFER_SIZE];
            snprintf(message, BUFFER_SIZE, "%d %d %d %02d:%02d %d", CODE_SEND_FINAL_SOLUTION, config.id_cliente, game_data->id_jogo, minutes, seconds, filled_count);
            strncat(message, game_data->tabuleiro, BUFFER_SIZE - strlen(message) - 1);
            if (send(client_socket, message, strlen(message), 0) == -1) {
                perror("Failed to send full solution to server");
            }
            printf("Full solution sent to the server.\n");
            log_event(config.log_file, config.id_cliente, CODE_SEND_FINAL_SOLUTION, "Full solution sent to the server.");
        } else {
            // Send the filled positions and numbers to the server
            char message[BUFFER_SIZE];
            snprintf(message, BUFFER_SIZE, "%d %d %d", CODE_SEND_PARTIAL_SOLUTION, config.id_cliente, game_data->id_jogo);
            for (int i = 0; i < filled_count; i++) {
                char pos_str[4];
                snprintf(pos_str, sizeof(pos_str), " %d", filled_positions[i]);
                strncat(message, pos_str, BUFFER_SIZE - strlen(message) - 1);
            }
            for (int i = 0; i < filled_count; i++) {
                char num_str[4];
                snprintf(num_str, sizeof(num_str), " %d", filled_numbers[i]);
                strncat(message, num_str, BUFFER_SIZE - strlen(message) - 1);
            }
            if (send(client_socket, message, strlen(message), 0) == -1) {
                perror("Failed to send partial solution to server");
                log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send partial solution to server.");
            } else {
                printf("Positions and numbers sent to the server.\n");
                char log_message[BUFFER_SIZE];
                snprintf(log_message, BUFFER_SIZE, "Partial solution with %d positions sent to the server.", filled_count);
                log_event(config.log_file, config.id_cliente, CODE_SEND_PARTIAL_SOLUTION, log_message);

                // Wait for the server's response
                ssize_t bytes_received = recv(client_socket, message, BUFFER_SIZE - 1, 0);
                if (bytes_received == -1) {
                    perror("Failed to receive response from server");
                    log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive response from server.");
                } else {
                    message[bytes_received] = '\0';
                    int response_code, error_count;
                    sscanf(message, "%d %d", &response_code, &error_count);
                    if (response_code == CODE_RESPONSE_INCORRECT_PARTIAL) {
                        printf("Partial solution has %d errors.\n", error_count);
                        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_INCORRECT_PARTIAL, "Partial solution has errors.");
                        // Replace incorrect positions with 'X'
                        char* token = strtok(message + strlen(message) + 1, " ");
                        for (int i = 0; i < error_count; i++) {
                            int error_pos;
                            sscanf(token, "%d", &error_pos);
                            game_data->tabuleiro[error_pos] = 'X';
                            token = strtok(NULL, " ");
                        }
                    } else if (response_code == CODE_RESPONSE_CORRECT_PARTIAL) {
                        printf("Partial solution is correct.\n");
                        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_CORRECT_PARTIAL, "Partial solution is correct.");
                    }
                }
            }
        }
    }
    game_in_progress = 0;
}

void display_game_status() {
    int filled_positions[81];
    int filled_numbers[81];
    int filled_count = 0;

    time_t now = time(NULL);
    double elapsed = difftime(now, hora_inicio);
    int minutes = (int)elapsed / 60;
    int seconds = (int)elapsed % 60;

    printf("\nID jogo a decorrer: %d\n", id_jogo);
    printf("Hora de início: %s", ctime(&hora_inicio));
    printf("Tempo decorrido: %02d:%02d\n", minutes, seconds);
    printf("Tabuleiro atual:\n");
    imprimirGrelha(tabuleiro);
}

//quando ele manda pro server e recebe do server, mostrar display