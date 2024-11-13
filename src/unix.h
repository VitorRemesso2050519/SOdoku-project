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

// Client-Side Codes
#define CODE_NEW_CLIENT 00                  // Client registers with the server
#define CODE_REQUEST_NEW_GAME 10            // Client requests a new Sudoku board
#define CODE_REQUEST_GAME_STATE 11          // Client requests the current game state (e.g., active players, time elapsed)
#define CODE_SEND_PARTIAL_SOLUTION 20       // Client submits a partial solution for verification
#define CODE_SEND_FINAL_SOLUTION 21         // Client submits a complete solution for verification
#define CODE_REQUEST_STATS 30               // Client requests game statistics, such as fastest completion time or records
#define CODE_DISCONNECT 99                  // Client requests to disconnect from the server

// Server-Side Codes
#define CODE_RESPONSE_NEW_GAME 100          // Server sends a new game board to the client
#define CODE_RESPONSE_GAME_STATE 101        // Server provides the current game state to the client
#define CODE_RESPONSE_DISCONNECT 102        // Server acknowledges client disconnection
#define CODE_RESPONSE_STATS 120             // Server provides the requested game statistics to the client
#define CODE_RESPONSE_OK 200                // Server responds indicating a successful operation

// Solution Verification Responses
#define CODE_RESPONSE_CORRECT_PARTIAL 110   // Server indicates that the submitted partial solution is correct
#define CODE_RESPONSE_INCORRECT_PARTIAL 111 // Server indicates that the submitted partial solution is incorrect
#define CODE_RESPONSE_CORRECT_FINAL 112     // Server indicates that the submitted final solution is correct
#define CODE_RESPONSE_INCORRECT_FINAL 113   // Server indicates that the submitted final solution is incorrect

// Competitive Multiplayer Updates
#define CODE_REQUEST_COMPETITION_CREATE 40  // Client requests to create a competitive multiplayer game
#define CODE_REQUEST_COMPETITION_JOIN 41    // Client requests to join a competitive multiplayer game
#define CODE_REQUEST_COMPETITION_START 42   // Client requests to start a competitive multiplayer game
#define CODE_REQUEST_COMPETITION_LEAVE 43   // Client requests to leave a competitive multiplayer game
#define CODE_NOTIFY_COMPETITION_CREATE 130  // Server notifies the client when a multiplayer game is created
#define CODE_NOTIFY_NEW_COMPETITOR 131      // Server notifies the client when a new competitor joins the game
#define CODE_NOTIFY_COMPETITION_LEAVE 132   // Server notifies the client when a competitor leaves the game
#define CODE_NOTIFY_COMPETITION_START 133   // Server notifies the client when the game starts
#define CODE_NOTIFY_COMPETITION_WINNER 139  // Server announces the winner of a multiplayer game, therefore ending competition
// Needs more codes probably

// Error and Control Codes
#define CODE_RESPONSE_INVALID_COMMAND 404   // Server notifies the client of an unrecognized or unsupported command
#define CODE_SERVER_SHUTDOWN 999            // Server signals impending shutdown to all connected clients
