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
int client_socket;
char tabuleiro[82]; // 81 characters + null terminator
time_t hora_inicio; // Declare hora_inicio
int id_jogo; // Declare id_jogo

void request_new_game();
void request_game_statistics();
void request_client_statistics();
void receive_new_game();
void receive_game_statistics();
void display_menu();
void display_game_status(); // Declare display_game_status before usage

int main(int argc, char* argv[]) {
    struct sockaddr_in server_addr;

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
                request_new_game(); // sends request to server asking for a new game
                break;
            case 2:
                display_game_status();
                break;
            case 3:
                request_game_statistics(); // sends request to server asking for info on the game client is currently playing
                break;
            case 4:
                request_client_statistics(); // grabs info from the config file and shows it to the user
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

void request_new_game() {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%d %d", CODE_REQUEST_NEW_GAME, config.id_cliente);
    if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
        perror("Failed to send new game request to server");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send new game request to server.");
    } else {
        printf("New game request sent to the server.\n");
        log_event(config.log_file, config.id_cliente, CODE_REQUEST_NEW_GAME, "New game request sent to the server.");
        receive_new_game();
    }
}

void request_game_statistics() {
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

void receive_new_game() {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received == -1) {
        perror("Failed to receive new game from server");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive new game from server.");
        return;
    }
    buffer[bytes_received] = '\0';

    int response_code, game_id, client_id;
    char new_board[82] = {0}; // Initialize with zeros to ensure null-termination

    int parsed_items = sscanf(buffer, "%d %d %d %81s", &response_code, &client_id, &game_id, new_board);
    if (parsed_items != 4) {
        printf("Failed to parse new game response. Parsed items: %d\n", parsed_items);
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to parse new game response.");
        return;
    }

    if (response_code == CODE_RESPONSE_NEW_GAME) {
        id_jogo = game_id;
        strncpy(tabuleiro, new_board, 82);
        hora_inicio = time(NULL);
        printf("New game received from server. Game ID: %d\n", id_jogo);
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_NEW_GAME, "New game received from server.");
    } else {
        printf("Failed to receive new game. Server response code: %d\n", response_code);
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive new game. Invalid response code.");
    }
}

void receive_game_statistics() {
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
    printf("2: View Current Game\n");
    printf("3: Request Current Game Statistics\n");
    printf("4: View Client Statistics\n");
    printf("0: Disconnect\n");
    printf("=============================================\n");
    printf("Enter command number: ");
}

void display_game_status() {
    int filled_positions[81];
    int filled_numbers[81];
    int filled_count = 0;

    while (1) {
        time_t now = time(NULL);
        double elapsed = difftime(now, hora_inicio);
        int minutes = (int)elapsed / 60;
        int seconds = (int)elapsed % 60;

        printf("\nID jogo a decorrer: %d\n", id_jogo);
        printf("Hora de início: %s", ctime(&hora_inicio));
        printf("Tempo decorrido: %02d:%02d\n", minutes, seconds);
        printf("Tabuleiro atual:\n");
        imprimirGrelha(tabuleiro);

        int pos, num;
        char command;
        printf("Enter 'I' to place a number, 'S' to save and send, or 0 to return to menu: ");
        scanf(" %c", &command);

        if (command == '0') {
            return;
        } else if (command == 'S') {
            // Check if the board is completely filled
            bool is_full = true;
            for (int i = 0; i < 81; i++) {
                if (tabuleiro[i] == '0') {
                    is_full = false;
                    break;
                }
            }

            if (is_full) {
                // Send the full solution to the server
                char message[BUFFER_SIZE];
                snprintf(message, BUFFER_SIZE, "%d %d %d %02d:%02d %d", CODE_SEND_FINAL_SOLUTION, config.id_cliente, id_jogo, minutes, seconds, filled_count);
                strncat(message, tabuleiro, BUFFER_SIZE - strlen(message) - 1);
                if (send(client_socket, message, strlen(message), 0) == -1) {
                    perror("Failed to send full solution to server");
                }
                printf("Full solution sent to the server.\n");
                log_event(config.log_file, config.id_cliente, CODE_SEND_FINAL_SOLUTION, "Full solution sent to the server.");
            } else {
                // Send the filled positions and numbers to the server
                char message[BUFFER_SIZE];
                snprintf(message, BUFFER_SIZE, "%d %d %d", CODE_SEND_PARTIAL_SOLUTION, config.id_cliente, id_jogo);
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
                }
                printf("Positions and numbers sent to the server.\n");
                char log_message[BUFFER_SIZE];
                snprintf(log_message, BUFFER_SIZE, "Partial solution with %d positions sent to the server.", filled_count);
                log_event(config.log_file, config.id_cliente, CODE_SEND_PARTIAL_SOLUTION, log_message);
            }
        } else if (command == 'I') {
            printf("Enter position (1-81): ");
            scanf("%d", &pos);
            printf("Enter number (1-9): ");
            scanf("%d", &num);
            if (preencherPosicao(tabuleiro, pos-1, num + '0')) {
                char log_message[BUFFER_SIZE];
                snprintf(log_message, BUFFER_SIZE, "Number %d placed at position %d.", num, pos);
                log_event(config.log_file, config.id_cliente, CODE_FILL_POSITION, log_message);
                printf("%s\n", log_message);
                filled_positions[filled_count] = pos;
                filled_numbers[filled_count] = num;
                filled_count++;
            } else {
                printf("Invalid move/position.\n");
            }
        } else {
            printf("Invalid input.\n");
        }
    }
}