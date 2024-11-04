# SOdoku-project
Projeto de SO de sudoku.

O que se tem de fazer de carry-over da primeira fase:
Um método próprio para resolver o sudoku no cliente. (Bruteforce ou outra merda).
Um método melhor para verificar a solução do sudoku (lembro-me que ele não gostou como estava).

Para a segunda fase (pedido ao GPT para me ajudar a perceber onde começar):

# Establish Client-Server Communication

Networking Setup: Begin by adding socket programming basics to allow for actual communication between the client and server. This includes:
- Setting up a listening socket on the server to accept connections from multiple clients.
- Establishing a connection from the client to the server’s IP and port.

Define Message Protocols: Implement functions for each type of message to be exchanged between the client and server (e.g., pedeJogo, enviaSolucao). Since each message will be passed over sockets, consider a simple protocol for serializing data (for instance, using delimiters or fixed message structures).

# Integrate Client and Server Interactions

Requesting and Receiving Games: Modify the client to request a game from the server instead of manually defining one in solucao_incompleta. The server should respond with a Sudoku board based on the client's request, which you can do by sending serialized data over the socket.

Solution Submission: Update the client so it sends its solution back to the server once a game is “solved” or a partial solution is available. On the server, replace the placeholder solution check with actual data from the client, calling verificarSolucao with this data to validate.

Response Feedback: Implement a basic feedback loop where the server responds to the client with whether the submitted solution is correct or incorrect. This response could be a simple “Certo/Errado” message as specified.
⠀
# Implement Improved Sudoku Validation

Optimize Solution Checking: The current brute-force validation in verificarSolucao works for comparing final solutions but could be inefficient if solutions are validated frequently. Modify verificarSolucao to:
- Immediately detect rule violations by checking rows, columns, and 3x3 regions as soon as new numbers are placed. This could help catch incorrect moves early without requiring a full comparison.

Partial Validation on Client: Before submitting, let the client check for obvious errors (e.g., repeated numbers in rows or columns) to avoid unnecessary network traffic.

Expand Logging and Error Handling

Enhanced Logs: The log_event function is in place, but consider expanding log entries to capture more detail, such as:
- Server-side logs for each client connection, game request, and solution submission.
- Client-side logs detailing each step in attempting to solve the game, including errors caught before submission.

Error Handling for Networking: Ensure socket operations are robust with error handling for scenarios like connection loss, timeout, and invalid data. This will improve stability as you transition from testing with predefined data to real-time networked interactions.

# Improve User Feedback and Interface

User Interface Enhancements: Right now, the client simply prints output to the console. Add clear prompts or visual distinctions for:
Game board updates as each cell is filled in.
Status messages for feedback on server communication (e.g., “Connected to server,” “Solution correct,” etc.).

Timing and Statistics: Track and display the time taken to solve each puzzle, or show additional metrics like the number of attempts. This would add depth and help meet project requirements.
⠀
# Prepare Documentation for Synchronization and Communication Choices

Message Structure Documentation: Begin documenting your message format (e.g., start each message with a header for the message type or use a standard delimiter) for the communication protocol.

Synchronization Plan: As the server needs to manage multiple clients, plan for concurrent request handling. For simplicity, you might initially use a multi-threaded approach where each client connection is managed in its own thread.
