#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <utils.h>
#include "unix.h"
#include <stdbool.h>

// Definir a estrutura de configuração do servidor
typedef struct {
    char path_jogos[256];  // Caminho para o ficheiro de jogos
    char log_file[256];    // Caminho para o ficheiro de log
    int max_clients;       // Número máximo de clientes suportados
    //probably size of multiplayer room
} ConfigServidor;

// Definir a estrutura de um jogo
typedef struct {
    int id_jogo;
    char tabuleiro[81];  // Grelha 9x9 linearizada
    char solucao[81];    // Solução correspondente
} Jogo;

typedef struct {
    int id_jogo;
    int client_id;
    int attempts;          // Solution attempts
    time_t record_time;    // Record time for the game
} JogoState;

// Função para ler o ficheiro de configuração do servidor
void lerConfiguracaoServidor(const char* ficheiroConfig, ConfigServidor* config) {
    FILE* fp = fopen(ficheiroConfig, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de configuração!\n");
        exit(1);
    }
    fscanf(fp, "PATH_JOGOS: %s\n", config->path_jogos);
    fscanf(fp, "PATH_LOGS: %s\n", config->log_file);
    fscanf(fp, "MAX_CLIENTS: %d\n", &config->max_clients);
    fclose(fp);

    // Print a configuração carregada para verificar
    printf("Configuração carregada: PATH_JOGOS = %s, LOG_FILE = %s, MAX_CLIENTS = %d\n", config->path_jogos, config->log_file, config->max_clients);
}

// Função para carregar os jogos a partir de um ficheiro
void carregarJogos(const char* ficheiroJogos, Jogo jogos[], int *num_jogos) {
    FILE* fp = fopen(ficheiroJogos, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de jogos!\n");
        exit(1);
    }

    char line[256];
    // Skip the first line (header)
    fgets(line, sizeof(line), fp);

    int id;
    char tabuleiro[81];
    char solucao[81];
    int num_jogos = 0;

    // Ler os jogos e as soluções do ficheiro
    while (fscanf(fp, "%d , %s , %s\n", &id, tabuleiro, solucao) != EOF) {
        jogos[*num_jogos].id_jogo = id;
        strcpy(jogos[*num_jogos].tabuleiro, tabuleiro);
        strcpy(jogos[*num_jogos].solucao, solucao);
        (*num_jogos)++;
    }
    fclose(fp);
    printf("%d jogos carregados com sucesso.\n", num_jogos);
}

// Function to read game statistics from the file
bool lerEstatisticasJogo(const char* ficheiroEstatisticas, int game_id, JogoState* jogoState) {
    FILE* fp = fopen(ficheiroEstatisticas, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de estatísticas!\n");
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        int id, client_id, attempts;
        char record_time_str[9];
        sscanf(line, "%d , %d , %8s , %d", &id, &client_id, record_time_str, &attempts);
        if (id == game_id) {
            jogoState->id_jogo = id;
            jogoState->client_id = client_id;
            jogoState->attempts = attempts;
            strptime(record_time_str, "%H:%M:%S", &jogoState->record_time);
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false; // Game ID not found
}

// Function to write game statistics to the file
bool escreverEstatisticasJogo(const char* ficheiroEstatisticas, JogoState* jogoState) {
    FILE* fp = fopen(ficheiroEstatisticas, "r+");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de estatísticas!\n");
        return false;
    }

    char line[256];
    long pos;
    while ((pos = ftell(fp)) != -1 && fgets(line, sizeof(line), fp)) {
        int id;
        sscanf(line, "%d", &id);
        if (id == jogoState->id_jogo) {
            fseek(fp, pos, SEEK_SET);
            char record_time_str[9];
            strftime(record_time_str, sizeof(record_time_str), "%H:%M:%S", localtime(&jogoState->record_time));
            fprintf(fp, "%d , %d , %s , %d\n", jogoState->id_jogo, jogoState->client_id, record_time_str, jogoState->attempts);
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false; // Game ID not found
}

bool verificarPosicao(char num, int pos, char solucao_correta[81]) {
    if (num == solucao_correta[pos]) {
        return true;
    } else {
        return false;
    }
}

int verificarJogoCompleto(char tabuleiro[81], char solucao_correta[81]) {
    int erro = 0;
    if (solucao_correta == tabuleiro) {
        return 0;
    } else {
        for (int i = 0; i < 81; i++) {
            if(!verificarPosicao(tabuleiro[i], i, solucao_correta)) {
                erro++;
            }
        }
        return erro;
    }
}

Jogo grabRandomGame(Jogo jogos[], int num_jogos){
    int random_index = rand() % num_jogos;
    return jogos[random_index];
}

/*int main(int argc, char* argv[]) {
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
}*/
