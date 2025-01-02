#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <utils.h>
#include "unix.h"
#include <semaphore.h>
#include <stdbool.h>

// Definir a estrutura de configuração do servidor
typedef struct {
    char path_jogos[256];  // Caminho para o ficheiro de jogos
    char log_file[256];    // Caminho para o ficheiro de log
    char path_stats[256];  // Caminho para o ficheiro de estatísticas
    int max_clients;       // Número máximo de clientes suportados
    int room_size;         // Tamanho da sala de competição
} ConfigServidor;

// Definir a estrutura de um jogo
typedef struct {
    int id_jogo;
    char tabuleiro[81];  // Grelha 9x9 linearizada
    char solucao[81];    // Solução correspondente
} Jogo;

// Game state structure
typedef struct {
    int client_id;         // Client identifier
    int id_jogo;           // Game identifier
    int attempts;          // Solution attempts
    double record_time;    // Record time for the game
} JogoState;

// Barrier structure
typedef struct {
    sem_t mutex;           // Mutex semaphore
    sem_t turnstile1;      // Turnstile 1 semaphore
    sem_t turnstile2;      // Turnstile 2 semaphore
    int count;             // Counter
    int num_threads;       // Number of threads
} Barrier;

// Function to read server configuration from a file
void lerConfiguracaoServidor(const char* ficheiroConfig, ConfigServidor* config) {
    FILE* fp = fopen(ficheiroConfig, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de configuração!\n");
        exit(1);
    }
    fscanf(fp, "PATH_JOGOS: %s\n", config->path_jogos);
    fscanf(fp, "PATH_LOGS: %s\n", config->log_file);
    fscanf(fp, "PATH_STATS: %s\n", config->path_stats);
    fscanf(fp, "MAX_CLIENTS: %d\n", &config->max_clients);
    fscanf(fp, "ROOM_SIZE: %d\n", &config->room_size);
    
    fclose(fp);

    // Print a configuração carregada para verificar
    printf("Configuração carregada: PATH_JOGOS = %s, LOG_FILE = %s, PATH_STATS=%s, MAX_CLIENTS = %d, ROOM_SIZE = %d\n", config->path_jogos, config->log_file, config->path_stats, config->max_clients, config->room_size);
}

// Function to load games from a file
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
    *num_jogos = 0;

    // Read games from the file
    while (fscanf(fp, "%d , %81s , %81s\n", &id, tabuleiro, solucao) != EOF) {
        jogos[*num_jogos].id_jogo = id;
        strcpy(jogos[*num_jogos].tabuleiro, tabuleiro);
        strcpy(jogos[*num_jogos].solucao, solucao);
        (*num_jogos)++;
    }
    fclose(fp);
    printf("%d jogos carregados com sucesso.\n", *num_jogos);
}

// Function to read game statistics from the file
bool lerEstatisticasJogo(const char* ficheiroEstatisticas, int game_id, JogoState* jogoState) {
    printf("Opening statistics file: %s\n", ficheiroEstatisticas); // Debug print
    FILE* fp = fopen(ficheiroEstatisticas, "r");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de estatísticas!\n");
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("Read line: %s", line); // Debug print
        int id, client_id, attempts;
        char record_time_str[9];
        if (sscanf(line, "%d , %d , %8s , %d", &id, &client_id, record_time_str, &attempts) == 4) {
            printf("Parsed values: id=%d, client_id=%d, record_time_str=%s, attempts=%d\n", id, client_id, record_time_str, attempts); // Debug print
            if (id == game_id) {
                jogoState->id_jogo = id;
                jogoState->client_id = client_id;
                jogoState->attempts = attempts;

                // Convert H:M:S to float (total seconds)
                int hours, minutes, seconds;
                sscanf(record_time_str, "%2d:%2d:%2d", &hours, &minutes, &seconds);
                jogoState->record_time = hours * 3600 + minutes * 60 + seconds;

                fclose(fp);
                printf("Game statistics found and parsed successfully.\n"); // Debug print
                printf("Game ID: %d, Client ID: %d, Record Time: %.2f, Attempts: %d\n", jogoState->id_jogo, jogoState->client_id, jogoState->record_time, jogoState->attempts); // Debug print
                return true;
            }
        }
    }

    fclose(fp);
    printf("Game ID not found in statistics file.\n"); // Debug print
    return false; // Game ID not found
}

// Function to write game statistics to the file
bool escreverEstatisticasJogo(const char* ficheiroEstatisticas, int client_id, int id_jogo, int attempts, double record_time) {
    FILE* fp = fopen(ficheiroEstatisticas, "r+");
    if (fp == NULL) {
        printf("Erro ao abrir o ficheiro de estatísticas!\n");
        return false;
    }
    printf("jogoState: id_jogo=%d, client_id=%d, record_time=%.2f, attempts=%d\n", id_jogo, client_id, record_time, attempts); // Debug print
    char line[256];
    long pos;
    while (fgets(line, sizeof(line), fp)) {
        pos = ftell(fp) - strlen(line);
        int game_id;
        sscanf(line, "%d", &game_id);
        printf("Read ID: %d, looking for ID: %d\n", game_id, id_jogo); // Debug print
        if (game_id == id_jogo) {
            fseek(fp, pos, SEEK_SET);

            // Convert float (total seconds) to H:M:S
            int hours = (int)record_time / 3600;
            int minutes = ((int)record_time % 3600) / 60;
            int seconds = (int)record_time % 60;
            char record_time_str[9];
            snprintf(record_time_str, sizeof(record_time_str), "%02d:%02d:%02d", hours, minutes, seconds);

            printf("Writing new statistics: %d , %d , %s , %d\n", id_jogo, client_id, record_time_str, attempts); // Debug print
            fprintf(fp, "%d , %d , %s , %d\n", id_jogo, client_id, record_time_str, attempts);
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    printf("Game ID not found in statistics file.\n"); // Debug print
    return false; // Game ID not found
}

// Function to verify if a specific position is correct
bool verificarPosicao(char num, int pos, char solucao_correta[81]) {
    printf("Verifying position: pos=%d, num=%c, expected=%c\n", pos, num, solucao_correta[pos]);
    if (num == solucao_correta[pos]) {
        return true;
    } else {
        return false;
    }
}

// Function to verify if a game is completely correct
int verificarJogoCompleto(char tabuleiro[81], char solucao_correta[81]) {
    int erro = 0;
    for (int i = 0; i < 81; i++) {
        if(!verificarPosicao(tabuleiro[i], i, solucao_correta)) {
            erro++;
        }
    }
        return erro;
}

// Function to randomly select a game
Jogo grabRandomGame(Jogo jogos[], int num_jogos){
    int random_index = rand() % num_jogos;
    return jogos[random_index];
}

// Function to initialize a barrier
void barrier_init(Barrier* barrier, int num_threads) {
    sem_init(&barrier->mutex, 0, 1);
    sem_init(&barrier->turnstile1, 0, 0);
    sem_init(&barrier->turnstile2, 0, 1);
    barrier->count = 0;
    barrier->num_threads = num_threads;
}

// Function to wait on a barrier
void barrier_wait(Barrier* barrier) {
    sem_wait(&barrier->mutex);
    barrier->count++;
    if (barrier->count == barrier->num_threads) {
        sem_wait(&barrier->turnstile2);
        sem_post(&barrier->turnstile1);
    }
    sem_post(&barrier->mutex);

    sem_wait(&barrier->turnstile1);
    sem_post(&barrier->turnstile1);

    sem_wait(&barrier->mutex);
    barrier->count--;
    if (barrier->count == 0) {
        sem_wait(&barrier->turnstile1);
        sem_post(&barrier->turnstile2);
    }
    sem_post(&barrier->mutex);

    sem_wait(&barrier->turnstile2);
    sem_post(&barrier->turnstile2);
}