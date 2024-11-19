#ifndef UTIL_STREAM_CLIENT_H
#define UTIL_STREAM_CLIENT_H

#include <stdbool.h>

// Client configuration structure
typedef struct {
    int id_cliente;        // Client identifier
    char server_ip[16];    // Server IP address
    char log_file[256];    // Path to log file
    int games_solved;
    int errors_sent;
} ConfigCliente;

// Function to read client configuration from a file
void lerConfiguracaoCliente(const char* ficheiroConfig, ConfigCliente* config);

// Helper function to check if placing a number in the Sudoku grid is valid
bool ehValido(const char tabuleiro[81], int pos, char num);

// Function to shuffle an array of characters
void shuffle(char *array, int n);

// Function to fill a position with a valid number
bool preencherPosicao(char* tabuleiro, int pos, char* numeros);

// Function to solve the Sudoku puzzle completely
bool resolverCompleto(char* tabuleiro, int pos, char* numeros);

// Function to solve the Sudoku puzzle incrementally
void resolverIncremental(char* tabuleiro, int n, const char* servidor_ip, int id_cliente, char* numeros);

// Function to attempt to solve the Sudoku puzzle
void tentarResolver(char tabuleiro[81], const char* log_file, ConfigCliente config);

#endif // UTIL_STREAM_CLIENT_H
