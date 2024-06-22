#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <termios.h>

#define BUFFER_SIZE 4096


int running = 1;

void* monitor_stdin(void* arg) {
    while (running) {
        if (getchar() == '\n') {
            running = 0;
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    pthread_t stdin_thread;
    pthread_create(&stdin_thread, NULL, monitor_stdin, NULL);

    int client_socket;

    if (argc == 2) {
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            perror("socket");
            exit(1);
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(atoi(argv[1]));
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));

        listen(server_socket, 10);

        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &len);
        close(server_socket);

    } else {
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            perror("socket");
            exit(1);
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        if (inet_aton(argv[1], &addr.sin_addr) == 0) {
            perror("inet_aton");
            exit(1);
        }
        addr.sin_port = htons(atoi(argv[2]));
        int ret = connect(client_socket,(struct sockaddr *)&addr, sizeof(addr));
        if (ret == -1) {
            perror("connect");
            exit(1);
        }
    }

    FILE *fp_rec;
    char *cmd_rec = "rec -t raw -b 16 -c 1 -e s -r 48000 - ";

    FILE *fp_ply;
    char *cmd_ply = "play -t raw -b 16 -c 1 -e s -r 48000 - ";
    if (client_socket != -1) {
        fp_rec = popen(cmd_rec, "r");
        fp_ply = popen(cmd_ply, "w");
    }

    char DATA_REC[BUFFER_SIZE];
    char DATA_PLY[BUFFER_SIZE];

    while (running) {
        int n_rec = fread(DATA_REC, 1, BUFFER_SIZE, fp_rec);
        if (n_rec == -1) {
            perror("read");
            exit(1);
        }
        if (n_rec == 0) {
            break;
        }
        send(client_socket, DATA_REC, n_rec, 0);

        int n_ply = recv(client_socket, DATA_PLY, BUFFER_SIZE, 0);
        if (n_ply == -1) {
            perror("recv");
            exit(1);
        }
        if (n_ply == 0) {
            break;
        }

        fwrite(DATA_PLY, 1, n_ply, fp_ply);
        // デバッグのためにファイルまたはコンソールに時間を出力
    }


    close(client_socket);
    pclose(fp_rec);
    pclose(fp_ply);

    pthread_join(stdin_thread, NULL);

    return 0;
}
