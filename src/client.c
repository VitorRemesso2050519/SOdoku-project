#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char ip_servidor[16];  // Endereço IP do servidor
    int id_cliente;        // Identificador do cliente
} ConfigCliente;

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