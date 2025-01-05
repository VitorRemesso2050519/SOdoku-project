#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Message codes

// Client-Side Codes
#define CODE_NEW_CLIENT 00                  // Client registers with the server
#define CODE_REQUEST_NEW_GAME 10            // Client requests a new Sudoku board
#define CODE_SEND_PARTIAL_SOLUTION 20       // Client submits a partial solution for verification
#define CODE_SEND_FINAL_SOLUTION 21         // Client submits a complete solution for verification
#define CODE_DISCONNECT 99                  // Client requests to disconnect from the server

// Server-Side Codes
#define CODE_RESPONSE_NEW_GAME 100          // Server sends a new game board to the client
#define CODE_RESPONSE_DISCONNECT 666        // Server acknowledges client disconnection
#define CODE_NEW_RECORD 201                 // Server notifies the client of a new record
#define CODE_NOT_RECORD 202                 // Server notifies the client that the submitted time is not a record
#define CODE_SERVER_SHUTDOWN 999            // Server signals impending shutdown to all connected clients

// Solution Verification Responses
#define CODE_RESPONSE_CORRECT_PARTIAL 110   // Server indicates that the submitted partial solution is correct
#define CODE_RESPONSE_INCORRECT_PARTIAL 111 // Server indicates that the submitted partial solution is incorrect
#define CODE_RESPONSE_CORRECT_FINAL 112     // Server indicates that the submitted final solution is correct
#define CODE_RESPONSE_INCORRECT_FINAL 113   // Server indicates that the submitted final solution is incorrect

// Competitive Multiplayer Updates
#define CODE_REQUEST_COMPETITION_JOIN 41    // Client requests to join a competitive multiplayer game
#define CODE_NOTIFY_COMPETITION_JOIN 130    // Server notifies the client that they have joined a multiplayer game
#define CODE_NOTIFY_COMPETITION_START 131   // Server notifies the client when the game starts
#define CODE_NOTIFY_COMPETITION_WINNER 138  // Server announces the winner of a multiplayer game, therefore ending competition
#define CODE_NOTIFY_COMPETITION_END 139     // Server notifies the client that the competition has ended

// Error and Control Codes
#define CODE_SIMULATION_DATA 420            // Client writes down simulation data
#define CODE_RESPONSE_ERROR 404            // Server indicates an error occurred during the operation
#define CODE_RESPONSE_OK 69               // Server responds indicating a successful operation

// Game Board Operations
#define CODE_FILL_POSITION 50               // Client fills a position in the Sudoku board
#define CODE_WRONG_NUMBER 51                // Server says position is incorrect