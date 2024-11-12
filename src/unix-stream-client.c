
#include "unix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUFFER_SIZE 256

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
        case CODE_RESPONSE_OK:
            printf("Server responded with OK.\n");
            break;
        case CODE_RESPONSE_ERROR:
            printf("Server responded with ERROR.\n");
            break;
        default:
            printf("Unknown response code received from server: %d\n", response_code);
            break;
    }
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

    // Example client interaction
    int command;
    printf("Enter command (1: Start Game, 2: Submit Move, 3: Quit Game): ");
    scanf("%d", &command);

    switch (command) {
        case 1:
            send_message(client_socket, CODE_START_GAME);
            break;
        case 2:
            send_message(client_socket, CODE_SUBMIT_MOVE);
            break;
        case 3:
            send_message(client_socket, CODE_QUIT_GAME);
            break;
        default:
            printf("Invalid command.\n");
            break;
    }

    // Close the client socket
    close(client_socket);
    printf("Disconnected from the server.\n");

    return 0;
}
