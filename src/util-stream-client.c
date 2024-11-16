#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// Estrutura de configuração do cliente
typedef struct {
    int id_cliente;        // Identificador do cliente
    char server_ip[16];    // IP do servidor
    char log_file[256];    // Caminho para o ficheiro de log
} ConfigCliente;

// Função para ler o ficheiro de configuração do cliente
void lerConfiguracaoCliente(const char* ficheiroConfig, ConfigCliente* config) {
    FILE* fp = fopen(ficheiroConfig, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de configuração!\n");
        exit(1);
    }
    // Ler a configuração
    fscanf(fp, "ID_CLIENTE: %d\n", &config->id_cliente);
    fscanf(fp, "IP_SERVIDOR: %s\n", config->server_ip);
    fscanf(fp, "PATH_LOGS: %s\n", config->log_file);
    fclose(fp);

    // Print a configuração carregada para verificar
    printf("Configuração carregada: ID_CLIENTE = %d, SERVER_IP = %s, LOG_FILE = %s\n", 
           config->id_cliente, config->server_ip, config->log_file);
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

void shuffle(char *array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        char temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

bool resolverSudoku(char tabuleiro[81], int pos) {
    if (pos == 81) {
        return true;
    }

    if (tabuleiro[pos] != '0') {
        return resolverSudoku(tabuleiro, pos + 1);
    }

    char numeros[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    shuffle(numeros, 9);

    for (int i = 0; i < 9; i++) {
        if (ehValido(tabuleiro, pos, numeros[i])) {
            tabuleiro[pos] = numeros[i];
            if (resolverSudoku(tabuleiro, pos + 1)) {
                return true;
            }
            tabuleiro[pos] = '0';
        }
    }

    return false;
}

void tentarResolver(char tabuleiro[81], const char* log_file, ConfigCliente config) {
    if (resolverSudoku(tabuleiro, 0)) {
        log_event(log_file, config.id_cliente, 0, "Sudoku resolvido com sucesso.");
    } else {
        log_event(log_file, config.id_cliente, 0, "Falha ao resolver o Sudoku.");
    }
}

/*int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <ficheiro_configuracao>\n", argv[0]);
        return 1;
    }

    // Inicializar a configuração do cliente
    ConfigCliente config;
    lerConfiguracaoCliente(argv[1], &config);
    log_event(config.log_file," - Configuração do cliente [id] carregada.");

    // Exemplo de solução correta e solução enviada pelo cliente (esta fase não envolve comunicação real)
    char solucao_correta[81] = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";  // Solução correta
    char solucao_incompleta[81] = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";  // Solução incompleta

    // Simular uma tentativa de resolução
    simularTentativa(solucao_incompleta, solucao_correta, config.log_file, config);

    // Verificar a solução do cliente
    log_event(config.log_file," - Verificando solução do cliente [id].");

    return 0;
}*/