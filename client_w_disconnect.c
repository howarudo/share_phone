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
#include <string.h>

#define CSERVER_PORT 70000
#define BUFFER_SIZE 4096

void check_stopper(int ichi);
void write_stopper();
void write_to_log(char* FILE_NAME, char* str, int num);

void check_stopper(int ichi) {
    // if stopper.txt has 1, then stop the program
    FILE *f = fopen("stopper.txt", "r");
    char c;
    if (f == NULL) {
        return;
    }
    c = fgetc(f);
    if (c == '1') {
        write_to_log("log.txt", "Stopper ni yotte process ga shuuryou saremashita!", getpid());
        exit(0);
    }
    fclose(f);
    return;
}

void write_stopper() {
    // Write to file "stopper.txt"
    FILE *f = fopen("stopper.txt", "w");
    fprintf(f, "1");
    fclose(f);
    return;
}

void write_to_log(char* FILE_NAME, char* str, int num) {
    FILE *f = fopen(FILE_NAME, "a");
    fprintf(f, "%s %d\n", str, num);
    fclose(f);
    return;
}


void* monitor_stdin(void* arg) {
    while (1) {
        if (getchar() == '\n') {
            write_stopper();
            break;
        }
    }
    return NULL;
}

void call(int s){
    write_to_log("log.txt", "Call hajimeta yo!", getpid());
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

    while(1){
        check_stopper(2);
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

    check_stopper(3);
    write_to_log("log.txt", "Call owatta yo!", getpid());
}

void lntrim(char *str) {
    char *p;
    p = strchr(str, '\n');
    if(p != NULL) {
        *p = '\0';
    }
}

int main(int argc, char** argv){
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }
    remove("log.txt");
    remove("stopper.txt");

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

    // disconnect from central server
    close(s);

    check_stopper(4);

    char* IP_addr = malloc(sizeof(char)*20);
    FILE * f_log = fopen("log.txt","w");
    while(fgets(IP_addr, 20, IP) != NULL) {
        check_stopper(5);
        fprintf(f_log,"%s\n",IP_addr);
        fprintf(f_log, "while ni kita yo!, Process NO. %d\n", getpid());

        write_to_log("log.txt", "ima kara fork suru yo!", getpid());
        int qid = fork();
        write_to_log("log.txt", "fork dekita yo!", getpid());
        // write to log that
        if(qid == 0){
            lntrim(IP_addr);
            int s = socket(PF_INET,SOCK_STREAM,0);
            struct sockaddr_in addr;

            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr(IP_addr);
            addr.sin_port = htons(CSERVER_PORT);
            int ret = connect(s,(struct sockaddr *)&addr, sizeof(addr));
            if (ret == -1){
                perror("connect");
            exit(1);
            }
            call(s);
        }
    }
    write_to_log("log.txt", "while nuketa yo!", getpid());


    FILE *f = fopen("./log_queue.txt", "a");
    fprintf(f, "dekita \n");
    fclose(f);
    int ss = socket(PF_INET, SOCK_STREAM,0);
    int a = CSERVER_PORT;

    write_to_log("log.txt", "kore kara while hairu yo!", getpid());
    while(1){
        write_to_log("log.txt", "while ni kita yo!", getpid());
        check_stopper(100);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(a);
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(ss, (struct sockaddr *)&addr, sizeof(addr));

        listen(ss,10);

        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);

        int s = accept(ss, (struct sockaddr *)&client_addr, &len);
        write_to_log("log.txt", "ima kara server toshite fork suru yo!", getpid());
        int pid = fork();
        check_stopper(101);
        if(pid == 0){
            write_to_log("log.txt", "Server toshite clal suru yo", getpid());
            call(s);
            check_stopper(6);
        }
        check_stopper(102);
        close(s);
    }
    write_to_log("log.txt", "saigo no while nuketa yo!", getpid());
    free(IP_addr);
    check_stopper(7);

    return 0;
}
