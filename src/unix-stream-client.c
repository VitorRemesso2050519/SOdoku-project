
#include "unix.h"
#include "util-stream-client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>  // Include for IP address handling

#define BUFFER_SIZE 512
#define PORT 8080

void send_message(int client_socket, int code) {
    // Send the code to the server
    if (send(client_socket, &code, sizeof(code), 0) == -1) {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
    }

    // Receive a response from the server
    int response_code;
    if (recv(client_socket, &response_code, sizeof(response_code), 0) == -1) {
        perror("Failed to receive response from server");
        exit(EXIT_FAILURE);
    }

    // Interpret the server's response
    switch (response_code) {
        case CODE_RESPONSE_NEW_GAME:
            //starts solving the game
            break;
        case CODE_RESPONSE_GAME_STATE:
            printf("Server has sent you the current game state.\n");
            break;
        case CODE_RESPONSE_DISCONNECT:
            printf("Server has disconnected you.\n");
            break;
        case CODE_RESPONSE_STATS:
            printf("Server has sent you the game statistics.\n");
            break;
        case CODE_RESPONSE_INCORRECT_FINAL:
            //Client will have to try again
            break;    
        case CODE_RESPONSE_CORRECT_FINAL:
            //Client chilling.
            break;
        case CODE_RESPONSE_INCORRECT_PARTIAL:
            //Client will have to try again
            break;
        case CODE_RESPONSE_CORRECT_PARTIAL:
            //Client chilling but continues.
            break;
        
        default:
            printf("Unknown response code received from server: %d\n", response_code);
            break;
    }
}

void display_menu() {
    printf("\n========== Sudoku Client Interface ==========\n");
    printf("1: Request New Game\n");
    printf("2: View Current Game\n");
    //In View Current Game, it will show the screen. Each input will have some type of wait, like half a second.
    //"User" will be able to stop the game by sending solution, may it be full or partial.
    //Client will send string with info: code, client id, game id, how many numbers to verify, numbers, positions.
    printf("3: Request Current Game Statistics\n");
    printf("4: Request Client Statistics\n");
    printf("=============================================\n");
    printf("Enter command number (1-4) or 0 to disconnect: ");
}

// Função que roda na thread em background
void* resolverThread(ConfigCliente config) {

    // Embaralha o array de números de 1 a 9
    char numeros[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    shuffle(numeros, 9); // Embaralha os números uma vez para ser usado durante a resolução

    if (config.FLAG_FULL_OR_PARTIAL) {
        printf("Iniciando resolução completa...\n");
        if (resolverCompleto(config.tabuleiro, 0, numeros)) {
            printf("Resolução completa: Tabuleiro resolvido com sucesso!\n");
        } else {
            printf("Falha ao resolver o tabuleiro completo.\n");
        }
    } else {
        printf("Iniciando resolução incremental...\n");
        resolverIncremental(tabuleiro, config.n_posicoes, config.servidor_ip, config.id_cliente, numeros);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    int client_socket;
    struct sockaddr_in server_addr;

    if (argc < 2) {
        printf("Uso: %s <ficheiro_configuracao>\n", argv[0]);
        return 1;
    }

    // Inicializar a configuração do cliente
    ConfigCliente config;
    lerConfiguracaoCliente(argv[1], &config);
    //log_event();

    // Create a socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    if (inet_pton(AF_INET, config.server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server.\n");

    int command;

    while (1) {
        display_menu();
        scanf("%d", &command);
        switch (command) {
            case 1:
                send_message(client_socket, CODE_REQUEST_NEW_GAME);
                pthread_t thread_resolver;
                pthread_create(&thread_resolver, NULL, resolverThread(config), &config);

                printf("Resolução iniciada no background.\n");

                pthread_join(thread_resolver, NULL);
                break;
            case 2:
                //send_message(client_socket, CODE_SEND_FINAL_SOLUTION);
                break;
            case 3:
                send_message(client_socket, CODE_REQUEST_STATS);
                break;
            case 4:
                //This one just gets info from the client.config file, makes sense right?
                break;
            case 0:
                send_message(client_socket, CODE_DISCONNECT);
                break;
            default:
                send_message(client_socket, CODE_RESPONSE_INVALID_COMMAND);
                break;
        }
    }

    // Close the client socket
    close(client_socket);
    printf("Disconnected from the server.\n");

    return 0;
}
