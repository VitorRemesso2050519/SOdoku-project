#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <utils.h>
#include <stdbool.h>

// Definir a estrutura de configuração do servidor
typedef struct {
    char path_jogos[256];  // Caminho para o ficheiro de jogos
    char log_file[256];    // Caminho para o ficheiro de log
} ConfigServidor;

// Definir a estrutura de um jogo
typedef struct {
    int id_jogo;
    char tabuleiro[81];  // Grelha 9x9 linearizada
    char solucao[81];    // Solução correspondente
} Jogo;

#define MAXLINE 512

// Função para ler o ficheiro de configuração do servidor
void lerConfiguracaoServidor(const char* ficheiroConfig, ConfigServidor* config) {
    FILE* fp = fopen(ficheiroConfig, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de configuração!\n");
        exit(1);
    }
    fscanf(fp, "PATH_JOGOS: %s\n", config->path_jogos);
    fscanf(fp, "PATH_LOGS: %s\n", config->log_file); 
    fclose(fp);

    // Print a configuração carregada para verificar
    printf("Configuração carregada: PATH_JOGOS = %s, LOG_FILE = %s\n", config->path_jogos, config->log_file);
}

// Função para carregar os jogos a partir de um ficheiro
void carregarJogos(const char* ficheiroJogos, Jogo jogos[], int* num_jogos) {
    FILE* fp = fopen(ficheiroJogos, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de jogos!\n");
        exit(1);
    }

    int id;
    char tabuleiro[81];
    char solucao[81];
    *num_jogos = 0;

    // Ler os jogos e as soluções do ficheiro
    while (fscanf(fp, "%d , %s , %s\n", &id, tabuleiro, solucao) != EOF) {
        jogos[*num_jogos].id_jogo = id;
        strcpy(jogos[*num_jogos].tabuleiro, tabuleiro);
        strcpy(jogos[*num_jogos].solucao, solucao);
        (*num_jogos)++;
    }
    fclose(fp);
    printf("%d jogos carregados com sucesso.\n", *num_jogos);
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

// Server function to verify the client's move on the board
const char* verificarMovimento(char tabuleiro[81], int pos, char num, const char solucao_correta[81]) {
    // Check if the position is empty (i.e., '0') before validating the move
    if (tabuleiro[pos] != '0') {
        return "Posição já preenchida.";
    }

    // Verify that the number matches the correct solution for the position
    if (solucao_correta[pos] != num) {
        return "Número incorreto para esta posição.";
    }

    // Check if the move is valid according to Sudoku rules
    if (!ehValido(tabuleiro, pos, num)) {
        return "Número viola as regras do Sudoku (linha, coluna ou região 3x3).";
    }

    // If the move is valid, update the board with the move and return success
    tabuleiro[pos] = num;
    return "Movimento válido.";
}

// Function to handle moves and validate on the server
void str_echo(int sockfd) {
    int n;
    char line[MAXLINE];
    char tabuleiro[81];          // This should be initialized to the current game board state
    const char solucao_correta[81] = "534678912..."; // Example solution, should match the current game

    for (;;) {
        // Read move from client in the format "position,number"
        n = readline(sockfd, line, MAXLINE);
        if (n == 0) {
            return;  // Connection closed by client
        } else if (n < 0) {
            err_dump("str_echo: readline error");
        }

        int pos;
        char num;
        sscanf(line, "%d,%c", &pos, &num);  // Parse the "position,number" format

        // Validate the move
        const char* feedback = verificarMovimento(tabuleiro, pos, num, solucao_correta);

        // Send feedback back to the client
        if (writen(sockfd, feedback, strlen(feedback)) != strlen(feedback)) {
            err_dump("str_echo: writen error");
        }
    }
}

int main(int argc, char* argv[]) {
    // Verificar se o ficheiro de configuração foi passado como argumento
    if (argc < 2) {
        printf("Uso: %s <ficheiro_configuracao>\n", argv[0]);
        return 1;
    }

    //coisas pro socket
    //temos que verificar o que o client quer fazer

    // Inicializar a configuração e os jogos
    ConfigServidor config;
    Jogo jogos[100];   // Suporte para até 100 jogos por simplicidade
    int num_jogos = 0;

    // Ler a configuração do servidor
    lerConfiguracaoServidor(argv[1], &config);

    // Carregar os jogos a partir do ficheiro de jogos especificado na configuração
    carregarJogos(config.path_jogos, jogos, &num_jogos);

    // Placeholder para a lógica do servidor - gestão de clientes, etc.
    printf("Servidor pronto para aceitar conexões...\n");

    // Exemplo de como logar um evento de jogo
    log_event(config.log_file, "- Servidor iniciado e pronto para aceitar conexões.");

    // Simular uma interação do cliente (no futuro será a partir da rede)
    int id_jogo = 1;  // Vamos pegar no primeiro jogo para o teste
    char solucao_cliente[81] = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";  // Exemplo de solução enviada pelo cliente
    
    printf("\nCliente [id] recebeu o Jogo ID: %d\n", id_jogo);
    imprimirGrelha(jogos[id_jogo-1].tabuleiro);

    printf("\nCliente [id] enviou a solução para o Jogo ID: %d\n", id_jogo);
    log_event(config.log_file, "- Cliente [id] enviou solução.");

    // Verificar a solução
    if (verificarSolucao(solucao_cliente, jogos[id_jogo-1].solucao) == 0) {
        printf("\nSolução correta!\n");
        imprimirGrelha(solucao_cliente);
        log_event(config.log_file, "- Solução do cliente [id] correta.");
    } else {
        printf("\nSolução incorreta!\n");
        imprimirGrelha(solucao_cliente);
        log_event(config.log_file, "- Solução do cliente [id] incorreta.");
    }

    return 0;
}
