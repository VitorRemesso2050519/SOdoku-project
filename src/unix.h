#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define UNIXSTR_PATH "/tmp/s.unixstr"
#define UNIXDG_PATH  "/tmp/s.unixdgx"
#define UNIXDG_TMP   "/tmp/dgPROJETO"

// Message codes
#define CODE_START_GAME 1       // Client requests to start a new game
#define CODE_SUBMIT_MOVE 2      // Client submits a move
#define CODE_QUIT_GAME 3        // Client quits the game
#define CODE_RESPONSE_OK 200    // Server responds with OK
#define CODE_RESPONSE_ERROR 400 // Server responds with error