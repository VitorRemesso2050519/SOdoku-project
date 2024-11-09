#include "unix.h"
#include "utils.h"

int main(void) {
    int sockfd, newsockfd, clilen, childpid, servlen;
    struct sockaddr_un cli_addr, serv_addr;

    // Create UNIX domain stream socket
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        err_dump("server: can't open stream socket");

    // Prepare server address
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, UNIXSTR_PATH);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

    unlink(UNIXSTR_PATH); // Clean up any leftover socket file
    if (bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
        err_dump("server: can't bind local address");

    // Listen for client connections
    listen(sockfd, 5);

    for (;;) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            err_dump("server: accept error");

        if ((childpid = fork()) < 0)
            err_dump("server: fork error");
        else if (childpid == 0) { // Child process
            close(sockfd);        // Close listening socket in child
            str_echo(newsockfd);  // Process client moves
            exit(0);
        }

        // Parent process
        close(newsockfd); // Close connected socket in parent
    }
}