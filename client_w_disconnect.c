#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define CSERVER_PORT 50000
#define BUFFER_SIZE 4096
#include <string.h>

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

void call(int s){
    FILE* fp;
    FILE* fp2;
    char data[BUFFER_SIZE];
    char data_send[BUFFER_SIZE];

    char* cmd = "rec -t raw -b 16 -c 1 -e s -r 48000 -";
    fp = popen(cmd,"r");
    char* cmd2 = "play -t raw -b 16 -c 1 -e s -r 48000 -";
    fp2 = popen(cmd2,"w");

    pthread_t stdin_thread;
    pthread_create(&stdin_thread, NULL, monitor_stdin, NULL);

    while(running){
        int read_data = fread(data_send,1,BUFFER_SIZE,fp);
        if(read_data == -1){
            perror("read_data");
            exit(1);
        }else {
            if(read_data == 0){
                break;
            }else{
                send(s,data_send,BUFFER_SIZE,0);
            }
        }

        int n = recv(s,data,BUFFER_SIZE,0);
        if(n == -1){
            perror("recv");
            exit(1);
        }else{
            if(n == 0){
                break;
            }else{
                fwrite(data,1,BUFFER_SIZE,fp2);
            }
        }
    }
    //end test
    pthread_join(stdin_thread, NULL);
}

void lntrim(char *str) {
    char *p;
    p = strchr(str, '\n');
    if(p != NULL) {
        *p = '\0';
    }
}

int main(int argc,char** argv){
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    FILE * IP = fopen("IP.txt","w");

    int s = socket(PF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    int ret = connect(s,(struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1){
        perror("connect");
        exit(1);
    }

    while(1){
        char string[1];
        int n = recv(s,string,1,0);
        if(n == 0){
            break;
        }
        fwrite(string,1,1,IP);
    }
    fclose(IP);
    IP = fopen("IP.txt","r");
    if(IP == NULL){
        perror("fopen");
        exit(1);
    }


    char* IP_addr = malloc(sizeof(char)*20);
    while(fgets(IP_addr, 20, IP) != NULL) {
        FILE * f_log = fopen("log.txt","w");
        fprintf(f_log,"%s",IP_addr);
        int qid = fork();
        if(qid == 0){
            lntrim(IP_addr);
            int s = socket(PF_INET,SOCK_STREAM,0);
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr(IP_addr);
            addr.sin_port = htons(USER_PORT);
            int ret = connect(s,(struct sockaddr *)&addr, sizeof(addr));
            if (ret == -1){
                perror("connect");
            exit(1);
            }

            call(s);
        }
    }


    FILE *f = fopen("./log_queue.txt", "a");
    fprintf(f, "dekita \n");
    int ss = socket(PF_INET, SOCK_STREAM,0);

    while(1){
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(USER_PORT);
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(ss, (struct sockaddr *)&addr, sizeof(addr));

        listen(ss,10);

        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);

        int s = accept(ss, (struct sockaddr *)&client_addr, &len);
        fprintf(f, "Accept dekita \n");

        int pid = fork();
        if(pid == 0){
            call(s);
            if(running == 0){
                break;
            }
        }

    }
    free(IP_addr);


}