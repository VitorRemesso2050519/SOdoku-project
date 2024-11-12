#include "unix.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// Function to set up the server socket
int setup_server() {
    int sockfd;
    struct sockaddr_un serv_addr;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Error opening socket");
        exit(1);
    }

    // Configure server socket address
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, UNIXSTR_PATH);
    unlink(UNIXSTR_PATH); // Remove any leftover socket file

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket");
        close(sockfd);
        exit(1);
    }

    if (listen(sockfd, 5) < 0) {
        perror("Error listening on socket");
        close(sockfd);
        exit(1);
    }

    return sockfd;
}

int main() {
    int sockfd = setup_server(); // Set up server socket
    printf("Server is ready to accept connections...\n");

    while (1) {
        int client_fd;
        struct sockaddr_un cli_addr;
        socklen_t clilen = sizeof(cli_addr);

        if ((client_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0) {
            perror("Error accepting client connection");
            continue;
        }

        // Fork a child process to handle the client connection
        if (fork() == 0) { 
            close(sockfd); // Close listening socket in child process
            process_client_message(client_fd); // Call game handling function in util-stream-server.c
            close(client_fd); // Close client socket after handling
            exit(0); // End child process
        }
        close(client_fd); // Parent closes client socket
    }

    close(sockfd);
    return 0;
}
