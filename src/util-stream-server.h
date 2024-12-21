#ifndef UTIL_STREAM_SERVER_H
#define UTIL_STREAM_SERVER_H

#include <stdbool.h>
#include <time.h>
#include <semaphore.h>

// Server configuration structure
typedef struct {
    char path_jogos[256];  // Path to the game file
    char log_file[256];    // Path to log file
    char path_stats[256];  // Path to statistics file
    int max_clients;       // Maximum number of supported clients
    int room_size;         // Competition room size
} ConfigServidor;

// Game structure
typedef struct {
    int id_jogo;           // Game identifier
    char tabuleiro[81];    // Board layout as a 9x9 grid in a single string
    char solucao[81];      // Corresponding solution grid
} Jogo;

// Game state structure
typedef struct {
    int client_id;         // Client identifier
    int id_jogo;           // Game identifier
    int attempts;          // Solution attempts
    double record_time;    // Record time for the game
} JogoState;

typedef struct {
    sem_t mutex;
    sem_t turnstile1;
    sem_t turnstile2;
    int count;
    int num_threads;
} Barrier;

// Function to read server configuration from a file
void lerConfiguracaoServidor(const char* ficheiroConfig, ConfigServidor* config);

// Function to load games from a file
void carregarJogos(const char* ficheiroJogos, Jogo jogos[], int* num_jogos);

// Function to verify if a game is completely correct
int verificarJogoCompleto(char tabuleiro[81], char solucao_correta[81]);

// Function to verify if a specific position is correct
bool verificarPosicao(char tabuleiro[81], int pos, char solucao_correta[81]);

// Function to randomly select a game
Jogo grabRandomGame(Jogo jogos[], int num_jogos);

// Function to read game statistics from a file
bool lerEstatisticasJogo(const char* ficheiroEstatisticas, int game_id, JogoState* jogoState);

// Function to write game statistics to a file
bool escreverEstatisticasJogo(const char* ficheiroEstatisticas, int client_id, int id_jogo, int attempts, double record_time);

// Function to initialize a barrier
void barrier_init(Barrier* barrier, int num_threads);

// Function to wait on a barrier
void barrier_wait(Barrier* barrier);

#endif // UTIL_STREAM_SERVER_H
