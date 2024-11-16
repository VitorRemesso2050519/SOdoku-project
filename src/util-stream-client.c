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
    bool FLAG_FULL_OR_PARTIAL;
    int n_posicoes;
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
    fscanf(fp, "FLAG_FULL_OR_PARTIAL: %d\n", &config->FLAG_FULL_OR_PARTIAL);
    fscanf(fp, "PARTIAL_NUM: %d\n", &config->n_posicoes);
    fclose(fp);

    // Print a configuração carregada para verificar
    printf("Configuração carregada: ID_CLIENTE = %d, SERVER_IP = %s, LOG_FILE = %s\n, FLAG_FULL_OR_PARTIAL = %d, PARTIAL_NUM = %d\n", 
           config->id_cliente, config->server_ip, config->log_file, config->FLAG_FULL_OR_PARTIAL, config->n_posicoes);
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

// Função auxiliar: tenta preencher uma posição com um número válido
bool preencherPosicao(char* tabuleiro, int pos, char* numeros) {
    for (int i = 0; i < 9; i++) {
        if (ehValido(tabuleiro, pos, numeros[i])) {
            tabuleiro[pos] = numeros[i];
            return true;
        }
    }
    return false;
}

// Função para resolver o Sudoku completo
bool resolverCompleto(char* tabuleiro, int pos, char* numeros) {
    if (pos == 81) return true; // Tabuleiro completo

    if (tabuleiro[pos] != '0') return resolverCompleto(tabuleiro, pos + 1, numeros);

    // Preenche a posição com um número válido
    if (preencherPosicao(tabuleiro, pos, numeros)) {
        if (resolverCompleto(tabuleiro, pos + 1, numeros)) return true;
        tabuleiro[pos] = '0'; // Backtracking
    }

    return false;
}

// Função para resolver o Sudoku incrementalmente
void resolverIncremental(char* tabuleiro, int n, const char* servidor_ip, int id_cliente, char* numeros) {
    int preenchidos = 0;

    while (preenchidos < 81) {
        int posicoes[9] = {0};  // Até `n` posições por vez
        char numerosPreenchidos[9] = {0};
        int enviados = 0;

        for (int i = 0; i < 81 && enviados < n; i++) {
            if (tabuleiro[i] == '0') {
                if (preencherPosicao(tabuleiro, i, numeros)) {
                    posicoes[enviados] = i;
                    numerosPreenchidos[enviados] = tabuleiro[i];
                    enviados++;
                }
            }
        }

        // Simula envio ao servidor
        printf("Enviando %d posições ao servidor...\n", enviados);
        for (int i = 0; i < enviados; i++) {
            printf("Posição %d: %c\n", posicoes[i], numerosPreenchidos[i]);
        }

        // Simula resposta do servidor
        bool erro_detectado = false;
        for (int i = 0; i < enviados; i++) {
            if (rand() % 4 == 0) {  // Simula erro aleatório
                erro_detectado = true;
                tabuleiro[posicoes[i]] = '0'; // Corrige no tabuleiro
                printf("Erro na posição %d\n", posicoes[i]);
            }
        }

        if (!erro_detectado) {
            printf("Todos os %d números corretos!\n", enviados);
            preenchidos += enviados;
        }

        sleep(1); // Simula tempo de comunicação
    }
}

/*bool resolverSudoku(char tabuleiro[81], int pos) {
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
}*/

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