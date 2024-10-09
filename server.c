#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Definir a estrutura de configuração do servidor
typedef struct {
    char path_jogos[256];  // Caminho para o ficheiro de jogos
} ConfigServidor;

// Definir a estrutura de um jogo
typedef struct {
    int id_jogo;
    char tabuleiro[81];  // Grelha 9x9 linearizada
    char solucao[81];    // Solução correspondente
} Jogo;

// Função para ler o ficheiro de configuração do servidor
void lerConfiguracaoServidor(const char* ficheiroConfig, ConfigServidor* config) {
    FILE* fp = fopen(ficheiroConfig, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de configuração!\n");
        exit(1);
    }
    fscanf(fp, "PATH_JOGOS: %s\n", config->path_jogos);
    fclose(fp);
    printf("Configuração carregada: PATH_JOGOS = %s\n", config->path_jogos);
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

    // Mais tarde: iniciar a escuta de conexões dos clientes

    return 0;
}