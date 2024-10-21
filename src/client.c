#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

// Função para simular uma tentativa de resolução
void simularTentativa(char tabuleiro[81], char solucao[81], const char* log_file) {
    printf("Tentando resolver o Sudoku...\n");
    log_event(log_file, " - Cliente [id] tentando resolver o Sudoku.");
    for (int i = 0; i < 81; i++) {
        if (tabuleiro[i] == '0') {
            tabuleiro[i] = solucao[i];  // Preencher com a solução correta
            printf("Preenchendo posição %d com %c\n", i, solucao[i]);

            // Registrar cada preenchimento no log
            char log_message[64];
            snprintf(log_message, sizeof(log_message), " - Cliente [id] preenchendo posição %d com %c", i, solucao[i]);
            log_event(log_file, log_message);
        }
    }

    printf("Sudoku resolvido!\n");
    log_event(log_file, " - Sudoku do cliente [id] resolvido.");
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