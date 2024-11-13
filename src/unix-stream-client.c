
#include "unix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUFFER_SIZE 512

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
            printf("Server has given you a new game.\n");
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
        code CODE_RESPONSE_INCORRECT_PARTIAL:
            printf("Incorrect partial solution.\n");
            break;
        code CODE_RESPONSE_CORRECT_PARTIAL:
            printf("Correct partial solution.\n");
            break;
        code CODE_RESPONSE_INCORRECT_FINAL:
            printf("Incorrect final solution.\n");
            break;    
        code CODE_RESPONSE_CORRECT_FINAL:
            printf("Correct final solution.\n");
            break;
        
        default:
            printf("Unknown response code received from server: %d\n", response_code);
            break;
    }
}

void display_menu() {
    printf("\n========== Sudoku Client Interface ==========\n");
    printf("1: Request New Game\n");
    printf("2: Request Current Game State\n");
    printf("3: Send Partial Solution\n");
    printf("4: Send Final Solution\n");
    printf("5: Request Game Statistics\n");
    printf("6: Disconnect\n");
    printf("=============================================\n");
}

int main() {
    int client_socket;
    struct sockaddr_un server_addr;

    // Create a UNIX domain socket
    if ((client_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, UNIXSTR_PATH, sizeof(server_addr.sun_path) - 1);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server.\n");

    int command;

    while (1) {
        display_menu();
        printf("Enter command number (1-6): ");
        scanf("%d", &command);
        switch (command) {
            case 1:
                send_message(client_socket, CODE_REQUEST_NEW_GAME);
                break;
            case 2:
                send_message(client_socket, CODE_REQUEST_GAME_STATE);
                break;
            case 3:
                send_message(client_socket, CODE_REQUEST_STATS);
                break;
            case 4:
                send_message(client_socket, CODE_SEND_PARTIAL_SOLUTION);
                break;
            case 5:
                send_message(client_socket, CODE_SEND_FINAL_SOLUTION);
                break;
            case 6:
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
