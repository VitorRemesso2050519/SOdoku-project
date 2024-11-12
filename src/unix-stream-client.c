#include "unix.h"
#include "utils.h"
#include <string.h>

int start_game(int sockfd) {
    int code = CODE_START_GAME;
    return writen(sockfd, &code, sizeof(code));  // Send start game request
}

int submit_move(int sockfd, int position, char number) {
    int code = CODE_SUBMIT_MOVE;
    char buffer[MAXLINE];
    // Format message: code | position | number
    snprintf(buffer, sizeof(buffer), "%d,%d,%c", code, position, number);
    return writen(sockfd, buffer, strlen(buffer));  // Send move request with code
}

int quit_game(int sockfd) {
    int code = CODE_QUIT_GAME;
    return writen(sockfd, &code, sizeof(code));  // Send quit game request
}

int main() {
    int sockfd;
    struct sockaddr_un serv_addr;
    char buffer[MAXLINE];

    // Create UNIX domain stream socket
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        err_dump("client: can't open stream socket");

    // Configure server address
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, UNIXSTR_PATH);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        err_dump("client: can't connect to server");

    // Start the game
    if (start_game(sockfd) <= 0)
        err_dump("client: start_game error");

    // Submit a move as an example
    int position = 0;
    char number = '5';
    if (submit_move(sockfd, position, number) <= 0)
        err_dump("client: submit_move error");

    // Receive feedback from server
    int n = readline(sockfd, buffer, MAXLINE);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Feedback from server: %s\n", buffer);
    } else if (n < 0) {
        err_dump("client: readline error");
    }

    // Quit the game
    if (quit_game(sockfd) <= 0)
        err_dump("client: quit_game error");

    close(sockfd);
    return 0;
}
