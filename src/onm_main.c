//
// Created by ali on 9/20/23.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "config.h"
#include "onm_cli.h"
#include "onm_yang.h"

#define GET_NEW_MODE_STR(current_mod, new_mode) \
    strcat((char*)current_mod, strcat(":", new_mode))


int sock_listen() {
    int sockfd, new_sock;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    const int TRUE = 1;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error in socket. Exiting...");
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &TRUE, sizeof(sockfd)) < 0) {
        perror("setsockopt");
        return -1;

    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error in binding. Exiting...");
        exit(1);
    }

    // Listen for connections
    if (listen(sockfd, 10) == 0) {
        printf("Listening...\n");
    } else {
        perror("Error in listening. Exiting...");
        exit(1);
    }
    return sockfd;
}

int sock_accept(int sockfd) {
    int new_sock;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    addr_size = sizeof(new_addr);
    new_sock = accept(sockfd, (struct sockaddr *) &new_addr, &addr_size);
    return new_sock;
}



int main() {
    int ret ;
    ret = onm_cli_init();
    if (ret<0){
        printf("ERROR: failed to initialize cli: existing...\n");
        return -1;
    }
    ret = onm_yang_init();
    if (ret<0){
        printf("ERROR: failed to initialize yang context: existing...\n");
        return -1;
    }

    int sockfd = sock_listen();
    while (1) {
        int sockfdc = sock_accept(sockfd);
        handle_session(sockfdc);
    }
}
