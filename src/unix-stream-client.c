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
pthread_mutex_t solver_thread = PTHREAD_MUTEX_INITIALIZER;
int client_socket;
int current_game_id = -1;

typedef struct {
    int id_jogo;
    char tabuleiro[81]; // 81 characters
} GameData;

void request_new_game();
void request_game_statistics();
void request_client_statistics();
GameData receive_new_game();
void receive_game_statistics();
void display_menu();
void display_game_status();
void* solve_game_complete(void* arg);
void* solve_game_in_increments(void* arg);

int main(int argc, char* argv[]) {
    struct sockaddr_in server_addr;
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
    snprintf(buffer, BUFFER_SIZE, "%d %d", config.id_cliente, CODE_NEW_CLIENT);
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
                request_new_game();
                break;
            case 2:
                //multiplayer();
                break;
            case 3:
                request_game_statistics(); // sends request to server asking for info on the game client is currently playing
                break;
            case 4:
                request_client_statistics(); // grabs info from the config file and shows it to the user
                break;
            case 0:
                snprintf(buffer, BUFFER_SIZE, "%d %d", config.id_cliente, CODE_DISCONNECT);
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
    snprintf(buffer, BUFFER_SIZE, "%d %d", config.id_cliente, CODE_REQUEST_NEW_GAME);
    if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
        perror("Failed to send new game request to server");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send new game request to server.");
    } else {
        printf("New game request sent to the server.\n");
        log_event(config.log_file, config.id_cliente, CODE_REQUEST_NEW_GAME, "New game request sent to the server.");
        GameData* game_data = malloc(sizeof(GameData));
        if (game_data == NULL) {
            perror("Failed to allocate memory for game data");
            return;
        }
        *game_data = receive_new_game();
        if (game_data->id_jogo != 0) {
            if (config.is_full_or_partial == 0 ) {
                pthread_create(&solver_thread, NULL, solve_game_complete, game_data);
                pthread_detach(&solver_thread); // Detach the thread to avoid resource leaks
            } else if (config.is_full_or_partial == 1) {
                pthread_create(&solver_thread, NULL, solve_game_in_increments, game_data);
                pthread_detach(&solver_thread); // Detach the thread to avoid resource leaks
            }
        } else {
            free(game_data);
        }
    }
}

GameData receive_new_game(){
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received == -1) {
        perror("Failed to receive new game from server");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive new game from server.");
        return (GameData){0};
    }

    buffer[bytes_received] = '\0';
    int response_code, client_id, game_id;
    char tabuleiro[81];
    sscanf(buffer, "%d %d %d %s", &client_id, &response_code, &game_id, tabuleiro);

    if (response_code == CODE_RESPONSE_NEW_GAME) {
        printf("New game received from the server.\n");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_NEW_GAME, "New game received from the server.");
        GameData game_data;
        game_data.id_jogo = game_id;
        memcpy(game_data.tabuleiro, tabuleiro, 81);
        display_game_status(game_data.tabuleiro, game_data.id_jogo);
        current_game_id = game_id; // Update the global variable
        return game_data;
    } else {
        printf("Failed to receive new game. Server response code: %d\n", response_code);
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive new game. Invalid response code.");
    }

    return (GameData){0};
}

void* solve_game_complete(void* arg) { //In theory, this function should solve the game in its entirety (not in increments)
    GameData* game_data = (GameData*)arg;
    char buffer[BUFFER_SIZE];
    int attempts = 0;

    char number_array[] = {'1','2','3','4','5','6','7','8','9'};
    shuffle(number_array, 9);

    time_t start_time = time(NULL);
    time_t end_time;
    double elapsed_time;
    int response_code;

    do {
        attempts++;
        for (int i = 0; i<82; i++) {
            if (game_data->tabuleiro[i] == '0') {
                for (int j = 0; j < 9; j++) {
                    if (preencherPosicao(game_data->tabuleiro, i, number_array[j])) {
                        game_data->tabuleiro[i] = number_array[j];
                        char message[100]; // Allocate enough space for the message
                        snprintf(message, sizeof(message), "Filled position %d with number %c.", i, number_array[j]);
                        log_event(config.log_file, config.id_cliente, CODE_FILL_POSITION, message);
                        break;
                    }
                }
            }
        }
        end_time = time(NULL);
        elapsed_time = difftime(end_time, start_time);
        display_game_status(game_data->tabuleiro, game_data->id_jogo, elapsed_time, start_time);

        
        printf("\nSudoku puzzle solved.\n");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_OK, "Sudoku puzzle solved.");
        snprintf(buffer, BUFFER_SIZE, "%d %d %d %s %d %.2f", config.id_cliente, CODE_SEND_FINAL_SOLUTION, game_data->id_jogo, game_data->tabuleiro, attempts, elapsed_time);
        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            perror("Failed to send solution to server");
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send solution to server.");
            return NULL;
        }

        // Wait for server response
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received == -1) {
            perror("Failed to receive game verification from server");
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive game verification from server.");
            return NULL;
        }
        buffer[bytes_received] = '\0';
        int client_id;
        sscanf(buffer, "%d %d", &client_id, &response_code);
        if (response_code == CODE_RESPONSE_CORRECT_FINAL) {
            printf("Final solution is correct!\n");
            display_game_status(game_data->tabuleiro, game_data->id_jogo, elapsed_time, start_time);
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_CORRECT_FINAL, "Final solution is correct.");

                // Wait for record status message from server
            bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received == -1) {
                perror("Failed to receive record status from server");
                log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive record status from server.");
                return NULL;
            }
            buffer[bytes_received] = '\0';
            int record_code;
            sscanf(buffer, "%d %d", &client_id, &record_code);
            if (response_code == CODE_NEW_RECORD) {
                printf("New record achieved!\n");
                log_event(config.log_file, config.id_cliente, CODE_NEW_RECORD, "New record achieved.");
            } else if (response_code == CODE_NOT_RECORD) {
                printf("Solution is correct but not a new record. Sorry!\n");
                log_event(config.log_file, config.id_cliente, CODE_NOT_RECORD, "Solution is correct but not a new record.");
            } else {
                printf("Failed to receive valid record status. Server response code: %d\n", response_code);
                log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive valid record status. Invalid response code.");
                return NULL;
            }
            break;
        } else if (response_code == CODE_RESPONSE_INCORRECT_FINAL) {
            int errors;
            sscanf(buffer, "%d %d %d %d %s", &client_id, &response_code, &game_data->id_jogo, &errors, game_data->tabuleiro);
            printf("Final solution is incorrect. Contained %d errors. Retrying...\n", errors); //Reference number of errors
            display_game_status(game_data->tabuleiro, game_data->id_jogo, elapsed_time, start_time);
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_INCORRECT_FINAL, "Final solution is incorrect. Retrying...");
        } else {
            printf("Failed to receive game verification. Server response code: %d\n", response_code);
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive game verification. Invalid response code.");
            return NULL;
        }
    
    } while (response_code != CODE_RESPONSE_CORRECT_FINAL);

    return NULL;
}

void* solve_game_in_increments(void* arg) {
    GameData* game_data = (GameData*)arg;
    char buffer[BUFFER_SIZE];
    int attempts = 0;
    char positions[config.partial_num];
    int filled_positions = 0;

    char number_array[] = {'1','2','3','4','5','6','7','8','9'};
    shuffle(number_array, 9);

    time_t start_time = time(NULL);
    time_t send_time;
    double elapsed_time;
    int response_code;

    do {
        attempts++;
        int positions_filled_this_round = 0;

        // Fill n spaces incrementally
        for (int i = 0; i < 81 && positions_filled_this_round < config.partial_num; i++) {
            if (game_data->tabuleiro[i] == '0') {
                for (int j = 0; j < 9; j++) {
                    if (preencherPosicao(game_data->tabuleiro, i, number_array[j])) {
                        positions_filled_this_round++;
                        positions[positions_filled_this_round] = i;
                        char message[100]; // Allocate enough space for the message
                        snprintf(message, sizeof(message), "Filled position %d with number %c.", i, number_array[j]);
                        log_event(config.log_file, config.id_cliente, CODE_FILL_POSITION, message);
                        break;
                    }
                }
            }
        }

        filled_positions += positions_filled_this_round;
        send_time = time(NULL);
        elapsed_time = difftime(send_time, start_time);

        if (filled_positions == 81) {
            // Send the full solution to the server
            snprintf(buffer, BUFFER_SIZE, "%d %d %d %s %d %.2f", config.id_cliente, CODE_SEND_FINAL_SOLUTION, game_data->id_jogo, game_data->tabuleiro, attempts, elapsed_time);
            display_game_status(game_data->tabuleiro, game_data->id_jogo, elapsed_time, start_time);
            if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                perror("Failed to send full solution to server");
                log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send full solution to server.");
                return NULL;
            }
        } else {
            // Send the partial solution to the server
            snprintf(buffer, BUFFER_SIZE, "%d %d %d %s %d", config.id_cliente, CODE_SEND_PARTIAL_SOLUTION, game_data->id_jogo, positions_filled_this_round);
            char* positions[positions_filled_this_round];
            char* numbers_in_positions[positions_filled_this_round];
            for (int i = 0; i < positions_filled_this_round; i++) {
                char pos_str[4], num_str[4];
                snprintf(pos_str, sizeof(pos_str), " %d", positions[i]);
                snprintf(num_str, sizeof(num_str), " %c", numbers_in_positions[i]);
                strncat(buffer, pos_str, BUFFER_SIZE - strlen(buffer) - 1);
                strncat(buffer, num_str, BUFFER_SIZE - strlen(buffer) - 1);
            }
            display_game_status(game_data->tabuleiro, game_data->id_jogo, elapsed_time, start_time);
            if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                perror("Failed to send partial solution to server");
                log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send partial solution to server.");
                return NULL;
            }
        }

        // Wait for server response
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received == -1) {
            perror("Failed to receive solution verification from server");
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive solution verification from server.");
            return NULL;
        }
        buffer[bytes_received] = '\0';
        int client_id;
        sscanf(buffer, "%d %d", &client_id, &response_code);

        if (response_code == CODE_RESPONSE_CORRECT_PARTIAL) {
            printf("Partial solution is correct.\n");
            display_game_status(game_data->tabuleiro, game_data->id_jogo, elapsed_time, start_time);
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_CORRECT_PARTIAL, "Partial solution is correct.");
        } else if (response_code == CODE_RESPONSE_INCORRECT_PARTIAL) {
            int errors;
            sscanf(buffer, "%d %d %d %s", &client_id, &response_code, &game_data->id_jogo, &errors);
            for (int i = 0; i < errors; i++) {
                int error_position;
                sscanf(buffer + 4 + sizeof(int) * 3 + i * sizeof(int), "%d", &error_position);
                char message[100]; // Allocate enough space for the message
                snprintf(message, sizeof(message), "Position %d with incorrect number.", error_position);
                log_event(config.log_file, config.id_cliente, CODE_WRONG_NUMBER, message);
                game_data->tabuleiro[error_position] = '0';
            }
            printf("Partial solution is incorrect. Contained %d errors.\n", errors);
            display_game_status(game_data->tabuleiro, game_data->id_jogo, elapsed_time, start_time);
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_INCORRECT_PARTIAL, "Partial solution is incorrect.");
            filled_positions -= positions_filled_this_round; // Rollback the filled positions
        } else if (response_code == CODE_RESPONSE_CORRECT_FINAL) {
            printf("Final solution is correct!\n");
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_CORRECT_FINAL, "Final solution is correct.");
            display_game_status(game_data->tabuleiro, game_data->id_jogo, elapsed_time, start_time);
            // Wait for record status message from server
            bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received == -1) {
                perror("Failed to receive record status from server");
                log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive record status from server.");
                return NULL;
            }
            buffer[bytes_received] = '\0';
            int record_code;
            sscanf(buffer, "%d %d", &client_id, &record_code);

            if (record_code == CODE_NEW_RECORD) {
                printf("New record achieved!\n");
                log_event(config.log_file, config.id_cliente, CODE_NEW_RECORD, "New record achieved.");
            } else if (record_code == CODE_NOT_RECORD) {
                printf("Solution is correct but not a new record. Sorry!\n");
                log_event(config.log_file, config.id_cliente, CODE_NOT_RECORD, "Solution is correct but not a new record.");
            } else {
                printf("Failed to receive valid record status. Server response code: %d\n", record_code);
                log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive valid record status. Invalid response code.");
                return NULL;
            }
            break;
        } else if (response_code == CODE_RESPONSE_INCORRECT_FINAL) {
            int errors;
            sscanf(buffer, "%d %d %d %d %s", &client_id, &response_code, &game_data->id_jogo, &errors, game_data->tabuleiro);
            printf("Final solution is incorrect. Contained %d errors. Retrying...\n", errors);
            display_game_status(game_data->tabuleiro, game_data->id_jogo, elapsed_time, start_time);
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_INCORRECT_FINAL, "Final solution is incorrect. Retrying...");
            filled_positions -= errors; // Rollback the filled positions
        } else {
            printf("Failed to receive solution verification. Server response code: %d\n", response_code);
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to receive solution verification. Invalid response code.");
            return NULL;
        }

    } while (response_code != CODE_RESPONSE_CORRECT_FINAL);

    return NULL;

}

void display_game_status(char* tabuleiro, int id_jogo, double elapsed, time_t start_time) {

    int minutes = (int)elapsed / 60;
    int seconds = (int)elapsed % 60;

    printf("\nID jogo a decorrer: %d\n", id_jogo);
    printf("Hora de início: %s", ctime(&(start_time)));
    printf("Tempo decorrido: %02d:%02d\n", minutes, seconds);
    printf("Tabuleiro atual:\n");
    imprimirGrelha(tabuleiro);

}//quando ele manda pro server e recebe do server, mostrar display

void request_game_statistics() {
     if (current_game_id == -1) {
        printf("No game is currently being played.\n");
        return;
    }

    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%d %d %d", config.id_cliente, CODE_REQUEST_STATS, current_game_id);
    if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
        perror("Failed to send game statistics request to server");
        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send game statistics request to server.");
    } else {
        printf("Game statistics request sent to the server.\n");
        log_event(config.log_file, config.id_cliente, CODE_REQUEST_STATS, "Game statistics request sent to the server.");
        receive_game_statistics();
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
    sscanf(buffer, "%d %d %d %s %d", &client_id, &response_code, &game_id, record_time_str, &attempts);

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
    printf("Errors: %d\n", config.errors_sent);
    printf("=============================================\n");
}

void display_menu() {
    printf("\n========== Sudoku Client Interface ==========\n");
    printf("1: Request New Game (Singleplayer)\n");
    printf("2: Multiplayer\n"); //Mudou!
    printf("3: Request Current Game Statistics\n");
    printf("4: View Client Statistics\n");
    printf("0: Disconnect\n");
    printf("=============================================\n");
    printf("Enter command number: ");
}