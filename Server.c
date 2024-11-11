#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Function prototypes
void display_server_status(int client_count, int active_games, int resolved_games);
void log_event(const char *event);
void handle_admin_commands();
int accept_client(int server_fd);
void *client_handler(void *arg);
void send_message(int socket, const char *message);
void receive_message(int socket, char *buffer, size_t size);

// Global variables for server state
int client_count = 0;
int active_games = 0;
int resolved_games = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void display_server_status(int client_count, int active_games, int resolved_games) {
    printf("\n==== Sudoku Server Status ====\n");
    printf("Connected Clients: %d\n", client_count);
    printf("Active Games: %d\n", active_games);
    printf("Resolved Games: %d\n", resolved_games);
    printf("-------------------------------\n");
}

void log_event(const char *event) {
    FILE *log_file = fopen("server.log", "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline character

    fprintf(log_file, "[%s] %s\n", timestamp, event);
    fclose(log_file);

    // Also print to console for real-time monitoring
    printf("[LOG] %s\n", event);
}

void handle_admin_commands() {
    char command[256];
    while (1) {
        printf("Server> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0; // Remove newline character

        if (strcmp(command, "show_clients") == 0) {
            pthread_mutex_lock(&lock);
            display_server_status(client_count, active_games, resolved_games);
            pthread_mutex_unlock(&lock);
        } else if (strcmp(command, "shutdown") == 0) {
            printf("Shutting down server...\n");
            break; // Exit loop to shut down server
        } else {
            printf("Unknown command: %s\n", command);
        }
    }
}

int accept_client(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        perror("Failed to accept client connection");
        return -1;
    }

    pthread_mutex_lock(&lock);
    client_count++;
    pthread_mutex_unlock(&lock);

    log_event("New client connected.");
    return client_fd;
}

void *client_handler(void *arg) {
    int client_fd = *(int*)arg;
    char buffer[BUFFER_SIZE];

    // Example: Receive a message from the client
    receive_message(client_fd, buffer, BUFFER_SIZE);
    printf("Received from client: %s\n", buffer);

    // Example: Send a response to the client
    send_message(client_fd, "Game board sent!");

    close(client_fd);

    pthread_mutex_lock(&lock);
    client_count--;
    pthread_mutex_unlock(&lock);

    log_event("Client disconnected.");
    return NULL;
}

void send_message(int socket, const char *message) {
    if (send(socket, message, strlen(message), 0) == -1) {
        perror("Failed to send message");
    }
}

void receive_message(int socket, char *buffer, size_t size) {
    int bytes_received = recv(socket, buffer, size - 1, 0);
    if (bytes_received <= 0) {
        perror("Failed to receive message");
        buffer[0] = '\0';
        return;
    }
    buffer[bytes_received] = '\0'; // Null-terminate the message
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    log_event("Server started. Waiting for clients...");

    pthread_t admin_thread;
    pthread_create(&admin_thread, NULL, (void*)handle_admin_commands, NULL);

    while (1) {
        int client_fd = accept_client(server_fd);
        if (client_fd >= 0) {
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, client_handler, &client_fd);
            pthread_detach(thread_id); // Allow automatic resource cleanup
        }
    }

    close(server_fd);
    pthread_mutex_destroy(&lock);
    return 0;
}
