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

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_socket;
int current_game_id = -1;

typedef struct {
    int id_jogo;
    char tabuleiro[81]; // 81 characters
    ConfigCliente* client_config;
} GameData;

typedef struct {
    ConfigCliente* client_config;
    char mode[20];
} ThreadArgs;

void request_new_game(ConfigCliente* client_config, int thread_socket);
GameData receive_new_game(ConfigCliente* client_config, int thread_socket);
void display_menu();
void display_game_status(char* tabuleiro, int id_cliente, int id_jogo, double elapsed, time_t start_time);
void multiplayerpvp(ConfigCliente* client_config, int thread_socket);
void* multi_client_thread(void* arg);
void solve_game_in_increments(GameData* game_data);

int main(int argc, char* argv[]) {

    srand(time(NULL));

    if (argc < 2) {
        printf("Uso: <ficheiro_configuracao> [num_clients] [singleplayer/multiplayer/incrementaltest]\n");
        return 1;
    }

    if (argc == 2) {
        // Solo client mode
        ConfigCliente config;

        // Inicializar a configuração do cliente
        lerConfiguracaoCliente(argv[1], &config);
        pthread_mutex_lock(&log_mutex);
        log_event(config.log_file, config.id_cliente, 0, "Client configuration loaded.");
        pthread_mutex_unlock(&log_mutex);

        // Original single client mode
        struct sockaddr_in server_addr;
        pthread_t solver_thread;

        // Create a socket
        if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Failed to create socket");
            pthread_mutex_lock(&log_mutex);
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to create socket.");
            pthread_mutex_unlock(&log_mutex);
            exit(EXIT_FAILURE);
        }

        // Set up the server address structure
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        if (inet_pton(AF_INET, config.server_ip, &server_addr.sin_addr) <= 0) {
            perror("Invalid address/ Address not supported");
            pthread_mutex_lock(&log_mutex);
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Invalid address/ Address not supported.");
            pthread_mutex_unlock(&log_mutex);
            exit(EXIT_FAILURE);
        }

        // Connect to the server
        if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Failed to connect to server");
            pthread_mutex_lock(&log_mutex);
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to connect to server.");
            pthread_mutex_unlock(&log_mutex);
            exit(EXIT_FAILURE);
        }
        printf("Connected to the server.\n");
        pthread_mutex_lock(&log_mutex);
        log_event(config.log_file, config.id_cliente, CODE_NEW_CLIENT, "Connected to the server.");
        pthread_mutex_unlock(&log_mutex);

        // Send the initial message to the server
        char buffer[BUFFER_SIZE];
        snprintf(buffer, BUFFER_SIZE, "%d %d", config.id_cliente, CODE_NEW_CLIENT);
        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            perror("Failed to send initial message to server");
            pthread_mutex_lock(&log_mutex);
            log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send initial message to server.");
            pthread_mutex_unlock(&log_mutex);
            exit(EXIT_FAILURE);
        }
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer after sending

        // Main loop to handle user commands
        int command;
        while (1) {
            display_menu();
            scanf("%d", &command);
            switch (command) {
                case 1:
                    request_new_game(&config, client_socket);
                    break;
                case 2:
                    multiplayerpvp(&config, client_socket);
                    break;
                case 3:
                    //request_game_statistics(); // sends request to server asking for info on the game client is currently playing
                    break;
                case 4:
                    //request_client_statistics(); // grabs info from the config file and shows it to the user
                    break;
                case 0:
                    snprintf(buffer, BUFFER_SIZE, "%d %d", config.id_cliente, CODE_DISCONNECT);
                    if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                        perror("Failed to send disconnect request to server");
                        pthread_mutex_lock(&log_mutex);
                        log_event(config.log_file, config.id_cliente, CODE_RESPONSE_ERROR, "Failed to send disconnect request to server.");
                        pthread_mutex_unlock(&log_mutex);
                    }
                    close(client_socket);
                    printf("Disconnected from the server.\n");
                    pthread_mutex_lock(&log_mutex);
                    log_event(config.log_file, config.id_cliente, CODE_DISCONNECT, "Disconnected from the server.");
                    pthread_mutex_unlock(&log_mutex);
                    return 0;
                default:
                    printf("Invalid command.\n");
                    break;
            }
        }
    } else if (argc == 4) {
        // Multi-client mode
        int num_clients = atoi(argv[2]);
        char* mode = argv[3];
        
        if (mode == NULL || (strcmp(mode, "singleplayer") != 0 && strcmp(mode, "multiplayer") != 0 && strcmp(mode, "incrementaltest") != 0)) {
            printf("Invalid mode.\n");
            return 1;
        }

        if (strcmp(mode, "incrementaltest") == 0) {
            num_clients = 81; // Set the number of clients to 81 for incremental testing
        }

        printf("Creating %d clients in %s mode.\n", num_clients, mode);

        pthread_t threads[num_clients];

        for (int i = 0; i < num_clients; i++) {
            //create config for each client
            //Requires a common log file for all clients
            ConfigCliente* client_config = malloc(sizeof(ConfigCliente));
            if (client_config == NULL) {
                perror("Failed to allocate memory for client configuration");
                exit(EXIT_FAILURE);
            }

            if (strcmp(mode, "incrementaltest") == 0) {
                client_config->id_cliente = i + 1;
                strcpy(client_config->server_ip, "127.0.0.1");
                strcpy(client_config->log_file, "logs/common.log");
                client_config->is_vip = 0; // Everyone is a normal client for incremental testing
                client_config->partial_num = i + 1; // Partial number is the client number
            } else {
                client_config->id_cliente = i + 1;
                strcpy(client_config->server_ip, "127.0.0.1");
                strcpy(client_config->log_file, "logs/common.log");
                client_config->is_vip = rand() % 2; // Randomize between 0 and 1
                client_config->partial_num = rand() % 81 + 1; // Randomize between 1 and 81
            }

            printf("Client %d created. Configs: IP=%s, LOG=%s, PARTIAL_NUM=%d, IS_VIP=%d\n", client_config->id_cliente, client_config->server_ip, client_config->log_file, client_config->partial_num, client_config->is_vip);

            // Create thread data
            ThreadArgs* thread_data = malloc(sizeof(ThreadArgs));
            if (thread_data == NULL) {
                perror("Failed to allocate memory for thread data");
                exit(EXIT_FAILURE);
            }
            thread_data->client_config = client_config;
            strcpy(thread_data->mode, mode);

            if (pthread_create(&threads[i], NULL, multi_client_thread, (void*)thread_data) != 0) {
                perror("Failed to create client thread");
                exit(EXIT_FAILURE);
            }

            sleep(3);
        }

        for (int i = 0; i < num_clients; i++) {
            pthread_join(threads[i], NULL);
        }

    } else {
        printf("Invalid number of arguments.\n");
        return 1;
    }

    return 0;
}

void* multi_client_thread(void* arg) {
    ThreadArgs* thread_data = (ThreadArgs*)arg;
    ConfigCliente* client_config = thread_data->client_config;
    char* mode = thread_data->mode;
    struct sockaddr_in server_addr;
    int thread_socket;

    // Create a socket for this thread
    if ((thread_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Failed to create socket");
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to create socket.");
        pthread_mutex_unlock(&log_mutex);
        pthread_exit(NULL);
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, client_config->server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Invalid address/ Address not supported.");
        pthread_mutex_unlock(&log_mutex);
        pthread_exit(NULL);
    }

    // Connect to the server
    if (connect(thread_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to connect to server");
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to connect to server.");
        pthread_mutex_unlock(&log_mutex);
        pthread_exit(NULL);
    }
    printf("%d - Connected to the server.\n", client_config->id_cliente);
    pthread_mutex_lock(&log_mutex);
    log_event(client_config->log_file, client_config->id_cliente, CODE_NEW_CLIENT, "Connected to the server.");
    pthread_mutex_unlock(&log_mutex);

    // Send the initial message to the server
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%d %d", client_config->id_cliente, CODE_NEW_CLIENT);
    if (send(thread_socket, buffer, strlen(buffer), 0) == -1) {
        perror("Failed to send initial message to server");
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to send initial message to server.");
        pthread_mutex_unlock(&log_mutex);
        pthread_exit(NULL);
    }
    memset(buffer, 0, BUFFER_SIZE); // Clear the buffer after sending

    printf("%d - Initial message sent to the server.\n", client_config->id_cliente);

    // Ensure only one request is sent
    if (strcmp(mode, "singleplayer") == 0) {
        printf("%d - Requesting new game in singleplayer mode.\n", client_config->id_cliente);
        request_new_game(client_config, thread_socket);
    } else if (strcmp(mode, "multiplayer") == 0 || strcmp(mode, "incrementaltest") == 0) {
        printf("%d - Joining multiplayer game.\n", client_config->id_cliente);
        multiplayerpvp(client_config, thread_socket);
    } else {
        printf("Invalid mode.\n");
        pthread_exit(NULL);
    }

    pthread_exit(NULL);
}

void request_new_game(ConfigCliente* client_config, int thread_socket) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%d %d", client_config->id_cliente, CODE_REQUEST_NEW_GAME);
    if (send(thread_socket, buffer, strlen(buffer), 0) == -1) {
        perror("Failed to send new game request to server");
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to send new game request to server.");
        pthread_mutex_unlock(&log_mutex);
    } else {
        printf("%d - New game request sent to the server.\n", client_config->id_cliente);
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_REQUEST_NEW_GAME, "New game request sent to the server.");
        pthread_mutex_unlock(&log_mutex);
        GameData* game_data = malloc(sizeof(GameData));
        if (game_data == NULL) {
            perror("Failed to allocate memory for game data");
            return;
        }
        *game_data = receive_new_game(client_config, thread_socket);
        if (game_data->id_jogo != 0) {
            game_data->client_config = client_config; // Set the client configuration
            solve_game_in_increments(game_data);
            /*pthread_t solver_thread;
            pthread_create(&solver_thread, NULL, solve_game_in_increments, game_data);
            pthread_detach(&solver_thread); // Detach the thread to avoid resource leaks*/
        } else {
            free(game_data);
        }
    }
    memset(buffer, 0, BUFFER_SIZE); // Clear the buffer after sending
}

GameData receive_new_game(ConfigCliente* client_config, int thread_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(thread_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received == -1) {
        perror("Failed to receive new game from server");
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to receive new game from server.");
        pthread_mutex_unlock(&log_mutex);
        return (GameData){0};
    }

    buffer[bytes_received] = '\0';
    printf("%d - Received buffer: %s\n", client_config->id_cliente, buffer);

    int response_code = 0, client_id = 0, game_id = 0;
    char tabuleiro[81];

    // Use sscanf to parse the integers and the tabuleiro separately
    sscanf(buffer, "%d %d %d %81s", &client_id, &response_code, &game_id, tabuleiro);

    if (response_code == CODE_RESPONSE_NEW_GAME) {
        printf("New game received from the server.\n");
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_NEW_GAME, "New game received from the server.");
        pthread_mutex_unlock(&log_mutex);
        GameData game_data;
        game_data.id_jogo = game_id;
        memcpy(game_data.tabuleiro, tabuleiro, 81);
        current_game_id = game_id; // Update the global variable
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer after processing
        return game_data;
    } else {
        printf("Failed to receive new game. Server response code: %d\n", response_code);
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to receive new game. Invalid response code.");
        pthread_mutex_unlock(&log_mutex);
    }
    memset(buffer, 0, BUFFER_SIZE); // Clear the buffer after processing
    return (GameData){0};
}

void multiplayerpvp(ConfigCliente* client_config, int thread_socket) {
    char buffer[BUFFER_SIZE];
    int client_id, response_code, game_id;
    char tabuleiro[81];
    snprintf(buffer, BUFFER_SIZE, "%d %d %d", client_config->id_cliente, CODE_REQUEST_COMPETITION_JOIN, client_config->is_vip);
    if (send(thread_socket, buffer, strlen(buffer), 0) == -1) {
        perror("Failed to send competition join request to server");
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to send competition join request to server.");
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    memset(buffer, 0, BUFFER_SIZE); // Clear the buffer after sending

    ssize_t bytes_received = recv(thread_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received == -1) {
        perror("Failed to receive competition join response from server");
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to receive competition join response from server.");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    sscanf(buffer, "%d %d", &client_id, &response_code);

    if(response_code == CODE_NOTIFY_COMPETITION_JOIN) {
        printf("%d - You have joined the competition.\n", client_id);
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_NOTIFY_COMPETITION_JOIN, "You have joined the competition.");
        pthread_mutex_unlock(&log_mutex);
    } else {
        printf("%d - Failed to join the competition or room is full. Server response code: %d\n", client_id, response_code);
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to join the competition. Invalid response code.");
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    memset(buffer, 0, BUFFER_SIZE); // Clear the buffer after processing

    bytes_received = recv(thread_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received == -1) {
        perror("Failed to receive competition start message from server");
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to receive competition start message from server.");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    sscanf(buffer, "%d %d %d %81s", &client_id, &response_code, &game_id, tabuleiro);
    if(response_code == CODE_NOTIFY_COMPETITION_START){
        printf("Competition has started.\n");
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_NOTIFY_COMPETITION_START, "Competition has started.");
        pthread_mutex_unlock(&log_mutex);

        GameData* game_data = malloc(sizeof(GameData));
        if (game_data == NULL) {
            perror("Failed to allocate memory for game data");
            return;
        }
        game_data->id_jogo = game_id;
        memcpy(game_data->tabuleiro, tabuleiro, 81);
        game_data->client_config = client_config; // Set the client configuration
        current_game_id = game_id; // Update the global variable
        solve_game_in_increments(game_data);
        /*pthread_t solver_thread;
        pthread_create(&solver_thread, NULL, solve_game_in_increments, game_data);
        pthread_detach(&solver_thread); // Detach the thread to avoid resource leaks*/

    } else {
        printf("Failed to start the competition. Server response code: %d\n", response_code);
        pthread_mutex_lock(&log_mutex);
        log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to start the competition. Invalid response code.");
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    memset(buffer, 0, BUFFER_SIZE); // Clear the buffer after processing
}

void solve_game_in_increments(GameData* game_data) {
    ConfigCliente* client_config = game_data->client_config; // Get the client configuration
    char buffer[BUFFER_SIZE], positions[client_config->partial_num], n_in_positions[client_config->partial_num];
    int attempts = 0;
    int filled_positions = 0;

    time_t start_time = time(NULL);
    time_t send_time;
    double elapsed_time;
    int response_code;

    char number_array[] = {'1','2','3','4','5','6','7','8','9'};
    shuffle(number_array, 9);
    bool attempted_numbers[81][9] = {false};
    display_game_status(game_data->tabuleiro, client_config->id_cliente, game_data->id_jogo, elapsed_time, start_time);

    for (int i = 0; i < 81; i++) {
        if (game_data->tabuleiro[i] != '0') {
            filled_positions++;
        }
    }

    do {
        attempts++;
        int positions_filled_this_round = 0;

        // Fill n spaces incrementally
        for (int i = 0; i < 81 && positions_filled_this_round < client_config->partial_num; i++) {
            if (game_data->tabuleiro[i] == '0') {
                for (int j = 0; j < 9; j++) {
                    if (!attempted_numbers[i][j] && preencherPosicao(game_data->tabuleiro, i, number_array[j])) {
                        positions[positions_filled_this_round] = i;
                        n_in_positions[positions_filled_this_round] = number_array[j];
                        positions_filled_this_round++;
                        char message[100]; // Allocate enough space for the message
                        snprintf(message, sizeof(message), "Filled position %d with number %c.", i, number_array[j]);
                        pthread_mutex_lock(&log_mutex);
                        log_event(client_config->log_file, client_config->id_cliente, CODE_FILL_POSITION, message);
                        pthread_mutex_unlock(&log_mutex);
                        sleep(1); // Sleep for a second to simulate solving time
                        break;
                    }
                }
            }
        }

        filled_positions = filled_positions + positions_filled_this_round;
        send_time = time(NULL);
        elapsed_time = difftime(send_time, start_time);

        if (filled_positions == 81) {
            // Send the full solution to the server
            snprintf(buffer, BUFFER_SIZE, "%d %d %d %d %.2f %81s", client_config->id_cliente, CODE_SEND_FINAL_SOLUTION, game_data->id_jogo, attempts, elapsed_time, game_data->tabuleiro);
            display_game_status(game_data->tabuleiro, client_config->id_cliente, game_data->id_jogo, elapsed_time, start_time);
            if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                perror("Failed to send full solution to server");
                pthread_mutex_lock(&log_mutex);
                log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to send full solution to server.");
                pthread_mutex_unlock(&log_mutex);
                return;
            }
            memset(buffer, 0, BUFFER_SIZE); // Clear the buffer after sending
        } else {
            // Send the partial solution to the server
            snprintf(buffer, BUFFER_SIZE, "%d %d %d %d", client_config->id_cliente, CODE_SEND_PARTIAL_SOLUTION, game_data->id_jogo, positions_filled_this_round);
            for (int i = 0; i < positions_filled_this_round; i++) {
                char pos_str[4], num_str[4];
                snprintf(pos_str, sizeof(pos_str), " %d", positions[i]);
                snprintf(num_str, sizeof(num_str), " %c", n_in_positions[i]);
                strncat(buffer, pos_str, BUFFER_SIZE - strlen(buffer) - 1);
                strncat(buffer, num_str, BUFFER_SIZE - strlen(buffer) - 1);
            }
            display_game_status(game_data->tabuleiro, client_config->id_cliente, game_data->id_jogo, elapsed_time, start_time);
            if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                perror("Failed to send partial solution to server");
                pthread_mutex_lock(&log_mutex);
                log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to send partial solution to server.");
                pthread_mutex_unlock(&log_mutex);
                return;
            }
            memset(buffer, 0, BUFFER_SIZE); // Clear the buffer after sending
        }

        // Wait for server response
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received == -1) {
            perror("Failed to receive solution verification from server");
            pthread_mutex_lock(&log_mutex);
            log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to receive solution verification from server.");
            pthread_mutex_unlock(&log_mutex);
            return;
        }
        buffer[bytes_received] = '\0';
        int client_id;
        sscanf(buffer, "%d %d", &client_id, &response_code);

        if (response_code == CODE_RESPONSE_CORRECT_PARTIAL) {
            display_game_status(game_data->tabuleiro, client_config->id_cliente, game_data->id_jogo, elapsed_time, start_time);
            pthread_mutex_lock(&log_mutex);
            log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_CORRECT_PARTIAL, "Partial solution is correct.");
            pthread_mutex_unlock(&log_mutex);
        } else if (response_code == CODE_RESPONSE_INCORRECT_PARTIAL) {
            int errors = 0;
            sscanf(buffer, "%d %d %d", &client_id, &response_code, &errors);
            int offset = 0;
            for (int i = 0; i < 3; i++) {
                while (buffer[offset] != ' ') offset++;
                offset++;
            }
            for (int i = 0; i < errors; i++) {
                int pos;
                sscanf(buffer + offset, "%d", &pos);
                game_data->tabuleiro[pos] = '0';
                while (buffer[offset] != ' ' && buffer[offset] != '\0') offset++;
                offset++;
                for (int j = 0; j < positions_filled_this_round; j++) {
                    if (positions[j] == pos) {
                        for (int k = 0; k < 9; k++) {
                            if (number_array[k] == n_in_positions[j]) {
                                attempted_numbers[pos][k] = true;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            display_game_status(game_data->tabuleiro, client_config->id_cliente, game_data->id_jogo, elapsed_time, start_time);
            pthread_mutex_lock(&log_mutex);
            log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_INCORRECT_PARTIAL, "Partial solution is incorrect.");
            pthread_mutex_unlock(&log_mutex);
            filled_positions -= errors; // Rollback the filled positions
        } else if (response_code == CODE_RESPONSE_CORRECT_FINAL) {
            pthread_mutex_lock(&log_mutex);
            log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_CORRECT_FINAL, "Final solution is correct.");
            pthread_mutex_unlock(&log_mutex);
            display_game_status(game_data->tabuleiro, client_config->id_cliente, game_data->id_jogo, elapsed_time, start_time);
            memset(buffer, 0, BUFFER_SIZE);

            bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received == -1) {
                perror("Failed to receive record status from server");
                pthread_mutex_lock(&log_mutex);
                log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to receive record status from server.");
                pthread_mutex_unlock(&log_mutex);
                return;
            }
            buffer[bytes_received] = '\0';
            int record_code;
            sscanf(buffer, "%d %d", &client_id, &record_code);

            if (record_code == CODE_NEW_RECORD) {
                pthread_mutex_lock(&log_mutex);
                log_event(client_config->log_file, client_config->id_cliente, CODE_NEW_RECORD, "New record achieved.");
                pthread_mutex_unlock(&log_mutex);
                printf("%d - New record achieved.\n", client_config->id_cliente);
            } else if (record_code == CODE_NOT_RECORD) {
                pthread_mutex_lock(&log_mutex);
                log_event(client_config->log_file, client_config->id_cliente, CODE_NOT_RECORD, "Solution is correct but not a new record.");
                pthread_mutex_unlock(&log_mutex);
                printf("%d - Solution is correct but not a new record.\n", client_config->id_cliente);
            } else {
                pthread_mutex_lock(&log_mutex);
                log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to receive valid record status. Invalid response code.");
                pthread_mutex_unlock(&log_mutex);
                return;
            }
            memset(buffer, 0, BUFFER_SIZE);
        } else if (response_code == CODE_NOTIFY_COMPETITION_WINNER) {
            pthread_mutex_lock(&log_mutex);
            log_event(client_config->log_file, client_config->id_cliente, CODE_NOTIFY_COMPETITION_WINNER, "Client has won and competition will end.");
            pthread_mutex_unlock(&log_mutex);
            break;
        } else if (response_code == CODE_NOTIFY_COMPETITION_END) {
            pthread_mutex_lock(&log_mutex);
            log_event(client_config->log_file, client_config->id_cliente, CODE_NOTIFY_COMPETITION_END, "Competition has ended.");
            pthread_mutex_unlock(&log_mutex);
            break;
        } else {
            pthread_mutex_lock(&log_mutex);
            log_event(client_config->log_file, client_config->id_cliente, CODE_RESPONSE_ERROR, "Failed to receive solution verification. Invalid response code.");
            pthread_mutex_unlock(&log_mutex);
            return;
        }

    } while (response_code != CODE_RESPONSE_CORRECT_FINAL && response_code != CODE_NOTIFY_COMPETITION_WINNER && response_code != CODE_NOTIFY_COMPETITION_END);  
    memset(buffer, 0, BUFFER_SIZE);
}

void display_game_status(char* tabuleiro, int id_cliente, int id_jogo, double elapsed, time_t start_time) {

    pthread_mutex_lock(&display_mutex);

    int minutes = (int)elapsed / 60;
    int seconds = (int)elapsed % 60;

    printf("\nCliente: %d\n", id_cliente);
    printf("ID jogo a decorrer: %d\n", id_jogo);
    printf("Hora de início: %s", ctime(&(start_time)));
    printf("Tempo decorrido: %02d:%02d\n", minutes, seconds);
    printf("Tabuleiro atual:\n");
    imprimirGrelha(tabuleiro);

    pthread_mutex_unlock(&display_mutex);

}

/*void request_game_statistics() {
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
}*/

/*void request_client_statistics() {
    printf("\n======== Sudoku Client Statistics =========\n");
    printf("Solved Games: %d\n", config.games_solved);
    printf("Errors: %d\n", config.errors_sent);
    printf("=============================================\n");
}*/

void display_menu() {
    printf("\n========== Sudoku Client Interface ==========\n");
    printf("1: Request New Game (Singleplayer)\n");
    printf("2: Multiplayer\n"); //Mudou!
    //printf("3: Request Current Game Statistics\n");
    //printf("4: View Client Statistics\n");
    printf("0: Disconnect\n");
    printf("=============================================\n");
    printf("Enter command number: ");
}