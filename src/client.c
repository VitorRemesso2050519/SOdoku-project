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
    fscanf(fp, "ID_CLIENTE: %d\n", &config->id_cliente);
    fscanf(fp, "SERVER_IP: %d\n", &config->server_ip);
    fscanf(fp, "LOG_FILE: %s\n", config->log_file); 
    fclose(fp);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <ficheiro_configuracao>\n", argv[0]);
        return 1;
    }

    // Inicializar a configuração do cliente
    ConfigCliente config;
    lerConfiguracaoCliente(argv[1], &config);
    log_event(config.log_file," - Configuração do cliente carregada.");

    // Exemplo de solução correta e solução enviada pelo cliente (esta fase não envolve comunicação real)
    char solucao_correta[81] = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";  // Solução correta
    char solucao_cliente[81] = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";  // Solução enviada pelo cliente

    // Verificar a solução do cliente
    log_event(config.log_file," - Verificando solução do cliente.");

    return 0;
}