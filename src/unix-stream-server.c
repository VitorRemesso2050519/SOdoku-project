
#include "unix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUFFER_SIZE 256

void handle_client(int client_socket) {
    int code;
    char buffer[BUFFER_SIZE];

    // Receive a message code from the client
    int bytes_received = recv(client_socket, &code, sizeof(code), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive message code");
        return;
    }

    switch (code) {
        case CODE_START_GAME:
            printf("Client requested to start a new game.\n");
            // Respond with OK
            code = CODE_RESPONSE_OK;
            send(client_socket, &code, sizeof(code), 0);
            break;

        case CODE_SUBMIT_MOVE:
            printf("Client submitted a move.\n");
            // Additional logic to handle the move could be placed here
            code = CODE_RESPONSE_OK;
            send(client_socket, &code, sizeof(code), 0);
            break;

        case CODE_QUIT_GAME:
            printf("Client requested to quit the game.\n");
            code = CODE_RESPONSE_OK;
            send(client_socket, &code, sizeof(code), 0);
            break;

        default:
            printf("Unknown command received from client.\n");
            code = CODE_RESPONSE_ERROR;
            send(client_socket, &code, sizeof(code), 0);
            break;
    }
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_un server_addr;

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
        handle_client(client_socket);
        close(client_socket);
        printf("Client disconnected.\n");
    }

    // Close the server socket
    close(server_socket);
    unlink(UNIXSTR_PATH); // Clean up the socket file

    return 0;
}
