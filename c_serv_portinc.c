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
#define PORT 50000

void handle_client(int client_socket, struct sockaddr_in client_addr);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(1);
    }

    int serv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_socket == -1) {
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

    if (bind(serv_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(serv_socket);
        exit(1);
    }

    if (listen(serv_socket, 10) == -1) {
        perror("listen");
        close(serv_socket);
        exit(1);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        int client_socket = accept(serv_socket, (struct sockaddr *)&client_addr, &len);

        if (client_socket == -1) {
            perror("accept");
            continue;
        }

        if (fork() == 0) {
            close(serv_socket); // Child process does not need the listening socket
            handle_client(client_socket, client_addr);
            close(client_socket);
            exit(0);
        }
        close(client_socket); // Parent process does not need the connected socket
    }

    close(serv_socket);
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
 // ポート番号