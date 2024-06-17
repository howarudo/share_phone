#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    int s;

    if (argc == 2) {
        /*  SERVER
            Accepts port as arg, starts a server on that port
            Send data from stdin to the client
            When accepts from ss, start rec with popen
        */
        int ss = socket(AF_INET, SOCK_STREAM, 0);
        if (ss == -1) {
            perror("socket");
            exit(1);
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(atoi(argv[1]));
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(ss, (struct sockaddr *)&addr, sizeof(addr));

        listen(ss, 10);

        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        s = accept(ss, (struct sockaddr *)&client_addr, &len);
        close(ss);

    } else {
        /* CLIENT
        Accepts ip add and port as arg, connects to server on that ip and port
        Send data from stdin to the client
        */
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == -1) {
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
        int ret = connect(s,(struct sockaddr *)&addr, sizeof(addr));
        if (ret == -1) {
            perror("connect");
            exit(1);
        }
    }

    FILE *fp_rec;
    char *cmd_rec = "rec -t raw -b 16 -c 1 -e s -r 48000 -";

    FILE *fp_ply;
    char *cmd_ply = "play -t raw -b 16 -c 1 -e s -r 48000 -";
    if (s != -1) {
        fp_rec = popen(cmd_rec, "r");
        fp_ply = popen(cmd_ply, "w");

    }

    char DATA_REC[BUFFER_SIZE];
    char DATA_PLY[BUFFER_SIZE];

    while (1) {
        // find the time taken for 1 while loop iteration
        clock_t start = clock();
        int n_rec = fread(DATA_REC, 1, BUFFER_SIZE, fp_rec);
        if (n_rec == -1) {
            perror("read");
            exit(1);
        }
        if (n_rec == 0) {
            break;
        }
        send(s, DATA_REC, n_rec, 0);

        int n_ply = recv(s, DATA_PLY, BUFFER_SIZE, 0);
        if (n_ply == -1) {
            perror("recv");
            exit(1);
        }
        if (n_ply == 0) {
            break;
        }
        fwrite(DATA_PLY, 1, n_ply, fp_ply);
        clock_t end = clock();
        double time_taken = ((double)end - start) / CLOCKS_PER_SEC;
        // print to file
        // FILE *f = fopen("time.txt", "a");
        // fprintf(f, "%f\n", time_taken);
        // fclose(f);
    }
    close(s);
    pclose(fp_rec);
    return 0;
}
