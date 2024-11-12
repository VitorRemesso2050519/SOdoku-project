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

// Recursive brute-force function to solve the Sudoku puzzle
bool tentarResolver(char tabuleiro[81], const char solucao[81], int pos, const char* log_file) {
    // Base case: If we reach the end, the puzzle is solved
    if (pos == 81) {
        return true;
    }

    // If the cell is already filled, move to the next cell
    if (tabuleiro[pos] != '0') {
        return tentarResolver(tabuleiro, solucao, pos + 1, log_file);
    }

    // Try numbers 1 to 9 in the current empty cell
    for (char num = '1'; num <= '9'; num++) {
        if (ehValido(tabuleiro, pos, num)) {
            tabuleiro[pos] = num;  // Place the number tentatively
            imprimirGrelha(tabuleiro);

            // Log the attempt
            char log_message[64];
            snprintf(log_message, sizeof(log_message), " - Cliente [id] tentando posição %d com %c", pos, num);
            log_event(log_file, log_message);

            // Recur to the next position
            if (tentarResolver(tabuleiro, solucao, pos + 1, log_file)) {
                return true;
            }

            // Backtrack if placing num didn't lead to a solution
            tabuleiro[pos] = '0';
        }
    }

    return false;  // No solution found for this path, backtrack
}

// Função para simular uma tentativa de resolução
void simularTentativa(char tabuleiro[81], const char solucao[81], const char* log_file, const ConfigCliente config) {
    printf("Tentando resolver o Sudoku...\n");
    log_event(log_file, " - Cliente %d tentando resolver o Sudoku.", config.id_cliente);

    if (tentarResolver(tabuleiro, solucao, 0, log_file)) {
        printf("Sudoku resolvido!\n");
        log_event(log_file, " - Sudoku do cliente %d resolvido.", config.id_cliente);
    } else {
        printf("Não foi possível resolver o Sudoku.\n");
        log_event(log_file, " - Cliente %d não conseguiu resolver o Sudoku.", config.id_cliente);
    }
}

int main(int argc, char* argv[]) {
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
    simularTentativa(solucao_incompleta, solucao_correta, config.log_file);

    // Verificar a solução do cliente
    log_event(config.log_file," - Verificando solução do cliente [id].");

    return 0;
}