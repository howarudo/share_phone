#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

// Arrays (in a file)
char IP_FILE_NAME[] = "ip_addresses.txt";

void handle_client(int client_socket, struct sockaddr_in client_addr);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(1);
    }

    int ss = socket(AF_INET, SOCK_STREAM, 0);
    if (ss == -1) {
        perror("socket");
        exit(1);
    }

    // delete the file
    remove(IP_FILE_NAME);

    FILE *f = fopen(IP_FILE_NAME, "w");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(ss, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(ss);
        exit(1);
    }

    if (listen(ss, 10) == -1) {
        perror("listen");
        close(ss);
        exit(1);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        int s = accept(ss, (struct sockaddr *)&client_addr, &len);

        if (s == -1) {
            perror("accept");
            continue;
        }

        if (fork() == 0) {
            close(ss); // Child process does not need the listening socket
            handle_client(s, client_addr);
            close(s);
            exit(0);
        }
        close(s); // Parent process does not need the connected socket
    }

    close(ss);
    return 0;
}

void handle_client(int client_socket, struct sockaddr_in client_addr) {
    char ip[16];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));

    // Send all IP addresses to client
    FILE *f = fopen(IP_FILE_NAME, "r");
    char line[16];
    while (fgets(line, sizeof(line), f)) {
        send(client_socket, line, strlen(line), 0);
    }
    fclose(f);

    // Write the IP address to the file
    f = fopen(IP_FILE_NAME, "a");
    fprintf(f, "%s\n", ip);
    fclose(f);
}
