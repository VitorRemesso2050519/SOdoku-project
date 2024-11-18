#ifndef UTIL_STREAM_SERVER_H
#define UTIL_STREAM_SERVER_H

#include <stdbool.h>
#include <time.h>

// Server configuration structure
typedef struct {
    char path_jogos[256];  // Path to the game file
    char log_file[256];    // Path to log file
} ConfigServidor;

// Game structure
typedef struct {
    int id_jogo;           // Game identifier
    char tabuleiro[81];    // Board layout as a 9x9 grid in a single string
    char solucao[81];      // Corresponding solution grid
} Jogo;

// Game state structure
typedef struct {
    int id_jogo;           // Game identifier
    int attempts;          // Solution attempts
    time_t record_time;    // Record time for the game
} JogoState;

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
bool escreverEstatisticasJogo(const char* ficheiroEstatisticas, JogoState* jogoState);

#endif // UTIL_STREAM_SERVER_H
