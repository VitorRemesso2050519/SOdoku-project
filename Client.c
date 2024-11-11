#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

// Function prototypes
void show_menu();
void display_board(int board[9][9]);
void get_solution_input(int *row, int *col, int *value);
void send_message(int socket, const char *message);
void receive_message(int socket, char *buffer, size_t size);

// usar a funçao para fazer a geraçao dos tabuleiros
int board[9][9] = {
    {5, 3, 0, 0, 7, 0, 0, 0, 0},
    {6, 0, 0, 1, 9, 5, 0, 0, 0},
    {0, 9, 8, 0, 0, 0, 0, 6, 0},
    {8, 0, 0, 0, 6, 0, 0, 0, 3},
    {4, 0, 0, 8, 0, 3, 0, 0, 1},
    {7, 0, 0, 0, 2, 0, 0, 0, 6},
    {0, 6, 0, 0, 0, 0, 2, 8, 0},
    {0, 0, 0, 4, 1, 9, 0, 0, 5},
    {0, 0, 0, 0, 8, 0, 0, 7, 9}
}; 

void show_menu() {
    printf("\n==== Sudoku Client ====\n");
    printf("1. Request a new game\n");
    printf("2. Submit solution\n");
    printf("3. View game statistics\n");
    printf("4. Exit\n");
    printf("Select an option: ");
}

void display_board(int board[9][9]) {
    printf("\n    1 2 3   4 5 6   7 8 9\n");
    printf("  -------------------------\n");
    for (int i = 0; i < 9; i++) {
        printf("%d | ", i + 1);
        for (int j = 0; j < 9; j++) {
            printf("%d ", board[i][j]);
            if ((j + 1) % 3 == 0) printf("| ");
        }
        printf("\n");
        if ((i + 1) % 3 == 0) printf("  -------------------------\n");
    }
}

void get_solution_input(int *row, int *col, int *value) {
    do {
        printf("Enter row (1-9), column (1-9), and value (1-9): ");
        if (scanf("%d %d %d", row, col, value) != 3) {
            printf("Invalid input. Try again.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }

        if (*row < 1 || *row > 9 || *col < 1 || *col > 9 || *value < 1 || *value > 9) {
            printf("Values must be between 1 and 9.\n");
        }

    } while (*row < 1 || *row > 9 || *col < 1 || *col > 9 || *value < 1 || *value > 9);

    *row -= 1; // Convert to zero-indexed for internal board representation
    *col -= 1;
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
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int option, row, col, value;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // Connect to localhost

    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server.\n");

    while (1) {
        show_menu();
        scanf("%d", &option);
        while (getchar() != '\n'); // Clear input buffer

        switch (option) {
            case 1: // Request new game
                send_message(client_fd, "REQUEST_GAME");
                receive_message(client_fd, buffer, BUFFER_SIZE);
                printf("Server: %s\n", buffer);
                break;

            case 2: // Submit solution
                display_board(board);
                get_solution_input(&row, &col, &value);
                board[row][col] = value; // Update local board

                // Send updated value to the server
                snprintf(buffer, BUFFER_SIZE, "SUBMIT_SOLUTION %d %d %d", row + 1, col + 1, value);
                send_message(client_fd, buffer);
                receive_message(client_fd, buffer, BUFFER_SIZE);
                printf("Server: %s\n", buffer);
                break;

            case 3: // View game statistics
                send_message(client_fd, "VIEW_STATS");
                receive_message(client_fd, buffer, BUFFER_SIZE);
                printf("Server: %s\n", buffer);
                break;

            case 4: // Exit
                printf("Exiting the game.\n");
                send_message(client_fd, "EXIT");
                close(client_fd);
                return 0;

            default:
                printf("Invalid option. Please try again.\n");
                break;
        }
    }

    close(client_fd);
    return 0;
}
