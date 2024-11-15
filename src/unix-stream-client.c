
#include "unix.h"
#include "util-stream-client.h"
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
        /*code CODE_RESPONSE_INCORRECT_PARTIAL:
            printf("Incorrect partial solution.\n");
            break;
        code CODE_RESPONSE_CORRECT_PARTIAL:
            printf("Correct partial solution.\n");
            break;*/
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
    printf("2: View Current Game\n");
    //In View Current Game, it will show the screen. Each input will have some type of wait, like half a second.
    //"User" will be able to stop the game by sending solution, may it be full or partial.
    //Client will send string with info: code, client id, game id, how many numbers to verify, numbers, positions.
    printf("3: Request Current Game Statistics\n");
    printf("4: Request Client Statistics\n")
    printf("=============================================\n");
    printf("Enter command number (1-4) or 0 to disconnect: ");
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
        scanf("%d", &command);
        switch (command) {
            case 1:
                send_message(client_socket, CODE_REQUEST_NEW_GAME);
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
