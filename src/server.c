#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Estrutura para armazenar os dados de um jogo de Sudoku
typedef struct {
    int id_jogo;
    char tabuleiro[82];      // Tabuleiro 9x9 do Sudoku (81 caracteres + '\0')
    char solucao_correta[82]; // Solução 9x9 do Sudoku (81 caracteres + '\0')
} JogoSudoku;

// Estrutura de configuração do servidor
typedef struct {
    char ficheiro_jogos[100]; // Nome do ficheiro contendo jogos e soluções
} ConfigServidor;

// Função para ler o ficheiro de configuração do servidor
void lerConfiguracaoServidor(const char* ficheiroConfig, ConfigServidor* config) {
    FILE* fp = fopen(ficheiroConfig, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de configuração!\n");
        exit(1);
    }
    fscanf(fp, "FICHEIRO_JOGOS: %s\n", config->ficheiro_jogos);
    fclose(fp);
}

// Função para carregar os jogos de Sudoku a partir de um ficheiro
int carregarJogos(const char* ficheiroJogos, JogoSudoku* jogos, int max_jogos) {
    FILE* fp = fopen(ficheiroJogos, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de jogos!\n");
        exit(1);
    }

    int count = 0;
    while (count < max_jogos && fscanf(fp, "%d,%81[^,],%81[^\n]\n", &jogos[count].id_jogo, jogos[count].tabuleiro, jogos[count].solucao_correta) == 3) {
        count++;
    }

    fclose(fp);
    return count;
}

// Função para registrar eventos em um ficheiro de log
void registrarEvento(const char* descricao) {
    FILE* log = fopen("servidor_log.txt", "a");
    if (log == NULL) {
        printf("Erro ao abrir o ficheiro de log!\n");
        return;
    }

    // Obter a hora atual
    time_t agora;
    time(&agora);
    char* tempoAtual = ctime(&agora);
    tempoAtual[strlen(tempoAtual) - 1] = '\0';  // Remover o '\n' final

    // Escrever no ficheiro de log
    fprintf(log, "[%s] %s\n", tempoAtual, descricao);
    fclose(log);
}

// Função para verificar se uma solução dada está correta
int verificarSolucao(const char* solucao_cliente, const char* solucao_correta) {
    int erros = 0;
    for (int i = 0; i < 81; i++) {
        if (solucao_cliente[i] != solucao_correta[i]) {
            printf("Erro na posição %d: esperado %c, obtido %c\n", i, solucao_correta[i], solucao_cliente[i]);
            erros++;
        }
    }
    return erros;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <ficheiro_configuracao>\n", argv[0]);
        return 1;
    }

    // Inicializar a configuração do servidor
    ConfigServidor config;
    lerConfiguracaoServidor(argv[1], &config);
    registrarEvento("Configuração do servidor carregada");

    // Carregar os jogos de Sudoku do ficheiro
    JogoSudoku jogos[100];  // Limite de 100 jogos
    int num_jogos = carregarJogos(config.ficheiro_jogos, jogos, 100);
    registrarEvento("Jogos de Sudoku carregados");

    printf("Servidor pronto com %d jogos carregados.\n", num_jogos);

    // Exemplo de verificação de solução (usando um jogo e uma solução de exemplo)
    char solucao_cliente[82] = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";  // Exemplo de solução do cliente
    registrarEvento("Verificando solução do cliente para o jogo 1");

    int erros = verificarSolucao(solucao_cliente, jogos[0].solucao_correta);

    // Registrar o resultado da verificação
    if (erros == 0) {
        registrarEvento("Solução correta para o jogo 1");
        printf("Solução correta para o jogo 1!\n");
    } else {
        registrarEvento("Solução incorreta para o jogo 1");
        printf("Solução incorreta para o jogo 1! %d erros encontrados.\n", erros);
    }

    return 0;
}
