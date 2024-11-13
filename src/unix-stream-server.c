#include "unix.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUFFER_SIZE 512

ConfigServidor config;

typedef struct {
    int code;
    int id_cliente;
    int id_jogo;
    char tabuleiro[81];
} ServerResponse;

typedef struct {
    int code;
    int id_cliente;
    int id_jogo;
    char solucaoCliente[81];
} ClientRequest;

typedef struct {
    int code;
    int id_jogo;
    char tabuleiro[81];    // Grelha 9x9 linearizada
    char solucao[81];      // Solução correspondente
    int attempts;          // Number of solution attempts made by the client.
    time_t start_time;     // When the client started the game.
    time_t end_time;       // When the client completed the game.
} JogoState;

void handle_client(int client_socket, int num_jogos, Jogo jogos[]) {
    int code;
    char buffer[BUFFER_SIZE];
    ClientRequest request;
    ServerResponse response;
    JogoState game_state;

    log_event(config.log_file, user_id, CODE_NEW_CLIENT, "Client connected");

    // Receive a message code from the client
    int bytes_received = recv(client_socket, &request, sizeof(ClientRequest), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive client request");
        return;
    }

    switch (request.code) {
        case CODE_REQUEST_NEW_GAME:
            printf("Client requested a new game.\n");

            // Fetch a random game from the available list
            Jogo new_game = grabRandomGame(jogos, num_jogos);  // Helper method to fetch a new game
            game_state = initialize_game_state(request.id_cliente, new_game); // Initialize game state (not done yet)

            // Populate the ServerResponse struct with the game information
            response.code = CODE_RESPONSE_NEW_GAME;
            response.id_cliente = request.id_cliente;
            response.id_jogo = game_state.id_jogo;
            strncpy(response.tabuleiro, game_state.tabuleiro, 81);

            // Send the response with the new game to the client
            send(client_socket, &response, sizeof(ServerResponse), 0);
            break;
        case CODE_REQUEST_GAME_STATE:
            printf("Client requested game state.\n");
            // Send the current game state to the client
            ServerResponse response;
            response.code = CODE_RESPONSE_GAME_STATE;
            response.id_cliente = 0;
            response.id_jogo = 0;
            response.tabuleiro = NULL;
            send(client_socket, &code, sizeof(code), 0);
            break;
        case CODE_SEND_PARTIAL_SOLUTION:
            printf("Client submitted a partial solution.\n");

            // Fetch the game state for this client and game
            game_state = find_game_state(request.id_jogo, request.id_cliente); // Helper to locate game state

            // Update the current board state in game_state with partial solution
            memcpy(game_state.tabuleiro, request.solucaoCliente, 81);  // Update board state
            game_state.attempts++;

            // Populate the response with a success code
            response.code = CODE_RESPONSE_PARTIAL_OK;
            response.id_cliente = request.id_cliente;
            response.id_jogo = request.id_jogo;

            send(client_socket, &response, sizeof(ServerResponse), 0);
            break;
        case CODE_SEND_FINAL_SOLUTION:
            printf("Client submitted a final solution.\n");

            // Retrieve the game state to compare the solution
            game_state = find_game_state(request.id_jogo, request.id_cliente);

            // Check if the submitted solution matches the correct one
            if (verificarJogoCompleto(request.solucaoCliente, game_state.solucao)) {  // Helper for verification
                response.code = CODE_RESPONSE_CORRECT_FINAL;
                game_state.solved = 1;
                game_state.end_time = time(NULL);  // Record completion time
            } else {
                response.code = CODE_RESPONSE_INCORRECT_FINAL;
            }

            response.id_cliente = request.id_cliente;
            response.id_jogo = request.id_jogo;
            send(client_socket, &response, sizeof(ServerResponse), 0);
            break;
        case CODE_REQUEST_STATS:
            printf("Client requested game statistics.\n");

            // Populate the response with game statistics; for example:
            response.code = CODE_RESPONSE_STATS;
            response.id_cliente = request.id_cliente;
            response.id_jogo = request.id_jogo;
            snprintf(response.tabuleiro, sizeof(response.tabuleiro),
                     "Attempts: %d, Start: %ld, End: %ld", 
                     game_state.attempts, game_state.start_time, game_state.end_time);

            send(client_socket, &response, sizeof(ServerResponse), 0);
            break;
        case CODE_DISCONNECT:
            printf("Client requested to disconnect.\n");
            response.code = CODE_RESPONSE_DISCONNECT;
            send(client_socket, &response, sizeof(ServerResponse), 0);
            close(client_socket);  // Close client connection
            break;
        //Multiplayer Commands will be introduced later
        default:
            printf("Unknown command received from client.\n");
            response.code = CODE_RESPONSE_INVALID_COMMAND;
            send(client_socket, &response, sizeof(ServerResponse), 0);
            break;
    }
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
        // Accept a client connection
        if ((client_socket = accept(server_socket, NULL, NULL)) == -1) {
            perror("Failed to accept connection");
            continue;
        }

        printf("Client connected.\n");
        handle_client(client_socket, jogos, num_jogos);
        close(client_socket);
        printf("Client disconnected.\n");
    }

    // Close the server socket
    close(server_socket);
    unlink(UNIXSTR_PATH); // Clean up the socket file

    return 0;
}
