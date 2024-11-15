#ifndef UTIL_STREAM_CLIENT_H
#define UTIL_STREAM_CLIENT_H

#include <stdbool.h>

// Client configuration structure
typedef struct {
    int id_cliente;        // Client identifier
    char server_ip[16];    // Server IP address
    char log_file[256];    // Path to log file
} ConfigCliente;

// Function to read client configuration from a file
void lerConfiguracaoCliente(const char* ficheiroConfig, ConfigCliente* config);

// Helper function to check if placing a number in the Sudoku grid is valid
bool ehValido(const char tabuleiro[81], int pos, char num);

// Function to shuffle an array of characters
void shuffle(char *array, int n);

// Recursive function to solve the Sudoku puzzle
bool tentarResolver(char tabuleiro[81], const char solucao[81], int pos, const char* log_file, const ConfigCliente config);

// Function to simulate a resolution attempt for the Sudoku puzzle
void simularTentativa(char tabuleiro[81], const char solucao[81], const char* log_file, const ConfigCliente config);

#endif // UTIL_STREAM_CLIENT_H
