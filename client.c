#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "3490"  // Port number to connect to
#define MAXDATASIZE 100 // Max number of bytes to receive at once

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buffer[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr,"Usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Loop through all results and connect to the first available
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connected to %s\n", s);

    freeaddrinfo(servinfo); // All done with this structure

    // Game loop: Keep guessing until the server confirms the correct guess
    while (1) {
        printf("Enter your guess: ");
        fgets(buffer, sizeof(buffer), stdin); // Get input from user

        // Send the guess to server
        if (send(sockfd, buffer, strlen(buffer), 0) == -1) {
            perror("send");
            break;
        }

        // Receive response from server
        numbytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (numbytes <= 0) {
            if (numbytes == 0) {
                printf("Server closed connection.\n");
            } else {
                perror("recv");
            }
            break;
        }

        buffer[numbytes] = '\0'; // Null-terminate the received string
        printf("Server: %s", buffer); // Display server response

        // If correct break the loop
        if (strstr(buffer, "Correct!") != NULL) {
            printf("You won! Closing connection...\n");
            break;
        }
    }

    close(sockfd); // Close the socket when game is over
    return 0;
}
