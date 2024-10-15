#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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

// Função para obter a hora atual como string
void get_current_time(char* buffer, size_t size) {
    time_t raw_time;
    struct tm* time_info;
    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(buffer, size, "[%H:%M:%S]", time_info);  // Formato: [HH:MM:SS]
}

// Função para logar eventos do servidor
void log_event(const char* ficheiroLog, const char* evento) {
    FILE* log_fp = fopen(ficheiroLog, "a");  // Abrir em modo de append
    if (log_fp == NULL) {
        perror("Erro ao abrir o ficheiro de log");
        return;
    }
    char time_buffer[20];
    get_current_time(time_buffer, sizeof(time_buffer));
    fprintf(log_fp, "%s %s\n", time_buffer, evento);  // Escrever o tempo + evento
    fclose(log_fp);
}

// Função para ler o ficheiro de configuração do servidor
void lerConfiguracaoServidor(const char* ficheiroConfig, ConfigServidor* config) {
    FILE* fp = fopen(ficheiroConfig, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de configuração!\n");
        exit(1);
    }
    fscanf(fp, "PATH_JOGOS: %s\n", config->path_jogos);
    fscanf(fp, "LOG_FILE: %s\n", config->log_file);  // Novo: adicionar ficheiro de log
    fclose(fp);
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

// Função para verificar a solução do Sudoku (simples tentativa e erro por agora)
int verificarSolucao(char* solucao_cliente, char* solucao_correta) {
    return strcmp(solucao_cliente, solucao_correta) == 0;
}

int main(int argc, char* argv[]) {
    // Verificar se o ficheiro de configuração foi passado como argumento
    if (argc < 2) {
        printf("Uso: %s <ficheiro_configuracao>\n", argv[0]);
        return 1;
    }

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
    log_event(config.log_file, "Servidor iniciado e pronto para aceitar conexões.");

    // Simular uma interação do cliente (no futuro será a partir da rede)
    int id_jogo = 1;  // Vamos pegar no primeiro jogo para o teste
    char solucao_cliente[81] = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";  // Exemplo de solução enviada pelo cliente
    
    printf("Cliente enviou a solução para o Jogo ID: %d\n", id_jogo);
    log_event(config.log_file, "Cliente enviou solução.");

    // Verificar a solução
    if (verificarSolucao(solucao_cliente, jogos[id_jogo-1].solucao)) {
        printf("Solução correta!\n");
        log_event(config.log_file, "Solução correta.");
    } else {
        printf("Solução incorreta!\n");
        log_event(config.log_file, "Solução incorreta.");
    }

    return 0;
}
