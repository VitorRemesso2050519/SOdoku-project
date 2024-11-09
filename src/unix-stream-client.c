#include "unix.h"
#include "utils.h"
#include <string.h>

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

    // Example move loop (simulate sending moves)
    int position = 0;
    char number = '5';
    snprintf(buffer, sizeof(buffer), "%d,%c", position, number);
    
    // Send move to server
    if (writen(sockfd, buffer, strlen(buffer)) != strlen(buffer))
        err_dump("client: writen error");

    // Receive feedback from server
    int n = readline(sockfd, buffer, MAXLINE);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Feedback from server: %s\n", buffer);
    } else if (n < 0) {
        err_dump("client: readline error");
    }

    close(sockfd);
    return 0;
}