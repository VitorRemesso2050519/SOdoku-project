#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
void lerConfiguracaoCliente(const char* ficheiroConfig, ConfigCliente* config) {
    FILE* fp = fopen(ficheiroConfig, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de configuração!\n");
        exit(1);
    }
    // Read configuration from file
    fscanf(fp, "ID_CLIENTE: %d\n", &config->id_cliente);
    fscanf(fp, "IP_SERVIDOR: %s\n", config->server_ip);
    fscanf(fp, "PATH_LOGS: %s\n", config->log_file);
    fscanf(fp, "IS_VIP: %d\n", &config->is_vip);
    fscanf(fp, "PARTIAL_NUM: %d\n", &config->partial_num);
    fclose(fp);

    // Print the loaded configuration for verification
    printf("Configuração carregada: ID_CLIENTE = %d, SERVER_IP = %s, LOG_FILE = %s\n", 
           config->id_cliente, config->server_ip, config->log_file);
    printf("PARTIAL_NUM = %d, IS_VIP = %d\n", 
           config->partial_num, config->is_vip);
}

// Helper function to check if placing a number at a specific position is valid
bool ehValido(const char tabuleiro[81], int pos, char num) {
    int row = pos / 9;
    int col = pos % 9;

    // Check row and column for duplicates
    for (int i = 0; i < 9; i++) {
        if (tabuleiro[row * 9 + i] == num || tabuleiro[i * 9 + col] == num) {
            return false;
        }
    }

    // Check 3x3 subgrid for duplicates
    int startRow = row / 3 * 3;
    int startCol = col / 3 * 3;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (tabuleiro[(startRow + i) * 9 + (startCol + j)] == num) {
                return false;
            }
        }
    }

    return true;
}

// Function to fill a position with a valid number
bool preencherPosicao(char* tabuleiro, int pos, char num) {
    if (tabuleiro[pos] != '0') {
        return false;
    }
    if (ehValido(tabuleiro, pos, num)) {
        tabuleiro[pos] = num;
        return true;
    }
    return false;
}

// Function to shuffle an array of characters
void shuffle(char *array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        char temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}