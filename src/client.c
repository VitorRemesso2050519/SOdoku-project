#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Estrutura de configuração do cliente
typedef struct {
    char ip_servidor[16];  // Endereço IP do servidor (não usado nesta fase)
    int id_cliente;        // Identificador do cliente
} ConfigCliente;

// Função para ler o ficheiro de configuração do cliente
void lerConfiguracaoCliente(const char* ficheiroConfig, ConfigCliente* config) {
    FILE* fp = fopen(ficheiroConfig, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de configuração!\n");
        exit(1);
    }
    fscanf(fp, "IP_SERVIDOR: %s\n", config->ip_servidor);
    fscanf(fp, "ID_CLIENTE: %d\n", &config->id_cliente);
    fclose(fp);
}

// Função para registrar eventos em um ficheiro de log
void registrarEvento(const char* descricao) {
    FILE* log = fopen("cliente_log.txt", "a");
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

// Função para verificar se a solução de Sudoku está correta
int verificarSolucao(const char* solucao, const char* solucao_correta) {
    int erros = 0;

    for (int i = 0; i < 81; i++) {
        if (solucao[i] != solucao_correta[i]) {
            printf("Erro na posição %d: esperado %c, obtido %c\n", i, solucao_correta[i], solucao[i]);
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

    // Inicializar a configuração do cliente
    ConfigCliente config;
    lerConfiguracaoCliente(argv[1], &config);
    registrarEvento("Configuração do cliente carregada");

    // Exemplo de solução correta e solução enviada pelo cliente (esta fase não envolve comunicação real)
    char solucao_correta[81] = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";  // Solução correta
    char solucao_cliente[81] = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";  // Solução enviada pelo cliente

    // Verificar a solução do cliente
    registrarEvento("Verificando solução do cliente");
    int erros = verificarSolucao(solucao_cliente, solucao_correta);

    // Registrar o resultado da verificação
    if (erros == 0) {
        registrarEvento("Solução correta");
        printf("Solução correta!\n");
    } else {
        registrarEvento("Solução incorreta");
        printf("Solução incorreta! %d erros encontrados.\n", erros);
    }

    return 0;
}
