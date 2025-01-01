#ifndef UTIL_STREAM_CLIENT_H
#define UTIL_STREAM_CLIENT_H

#include <stdbool.h>

// Client configuration structure
typedef struct {
    int id_cliente;        // Client identifier
    char server_ip[16];    // Server IP address
    char log_file[256];    // Path to log file
    bool is_vip;           // Flag to see if client is VIP
    int partial_num;       // Number of positions to fill before sending partial solution
} ConfigCliente;

// Function to read client configuration from a file
void lerConfiguracaoCliente(const char* ficheiroConfig, ConfigCliente* config);

// Helper function to check if placing a number in the Sudoku grid is valid
bool ehValido(const char tabuleiro[81], int pos, char num);

// Function to shuffle an array of characters
void shuffle(char *array, int n);

// Function to fill a position with a valid number
bool preencherPosicao(char* tabuleiro, int pos, char* numeros);

#endif // UTIL_STREAM_CLIENT_H
