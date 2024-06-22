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

#define USER_PORT 24000
#define BUFFER_SIZE 8192

// IP.txt IPアドレスを取得
// log.txt ログはなんでもここに
// stopper.txt ここに1が入ったら終了

void check_stopper(int ichi);
void write_stopper();
void write_to_log(char* FILE_NAME, char* str, int num);

void check_stopper(int ichi) {
    // if stopper.txt has 1, then stop the program
    FILE *stopfile_read = fopen("stopper.txt", "r");
    char c;
    if (stopfile_read == NULL) {
        return;
    }
    c = fgetc(stopfile_read);
    if (c == '1') {
        write_to_log("log.txt", "Stopper ni yotte process ga shuuryou saremashita!", getpid());
        exit(0);
    }
    fclose(stopfile_read);
    return;
}

void write_stopper() {
    // Write to file "stopper.txt"
    FILE *stopfile_write = fopen("stopper.txt", "w");
    fprintf(stopfile_write, "1");
    fclose(stopfile_write);
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
    FILE * f_log = fopen("log.txt","w");
    remove("stopper.txt");

    FILE * IP = fopen("IP.txt","w");

    // central serverと接続するためのソケットを作る
    int socket_to_CS = socket(PF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    // CS(central server)とconnect
    int ret = connect(socket_to_CS,(struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1){
        perror("connect");
        exit(1);
    }

    // すでにつながっているパソコンのIPアドレスをCSから読み込む
    while(1){
        char string[1];
        int n = recv(socket_to_CS,string,1,0);
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
    // CSとの接続を切断する
    close(socket_to_CS);

    // check_stopper(4);

    // CSからもらったIPアドレスを順に読み込む
    char* IP_addr = malloc(sizeof(char)*20);
    

    // すでにつながっているパソコンの数だけ（ここを検証するべき！！！！！！）
    while(fgets(IP_addr, 20, IP) != NULL) {
        check_stopper(5);

        fprintf(f_log,"[initial client] to shite %s wo yomi kondayo ! [Process No. %d]\n",IP_addr, getpid());
        fclose(f_log);
        
        write_to_log("log.txt", "[initial client] ima kara fork suru yo!", getpid());
        /* forkの分担 
        親プロセス: 次のIPアドレスを読み込んで 読み込み終わったら serverとしての役割にシフトする
        子プロセス: 読み込んだIPアドレスを使って通話をする
        */ 
        int qid = fork();
        if(qid == -1){
            perror("fork");
            exit(1);
        }
        write_to_log("log.txt", "[initial client] fork dekita yo!", getpid());
        // write to log that
        if(qid == 0){
            // 以下子プロセスの挙動
            // IPアドレスをトリムする
            lntrim(IP_addr);
            // phone serverと接続するためのソケットを作る
            int s_to_PS = socket(PF_INET,SOCK_STREAM,0);
            struct sockaddr_in addr;

            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr(IP_addr);
            addr.sin_port = htons(USER_PORT);
            // phone serverとconnect
            int ret = connect(s_to_PS,(struct sockaddr *)&addr, sizeof(addr));
            if (ret == -1){
                perror("connect");
            exit(1);
            }
            write_to_log("log.txt", "[initial client] PS to call hazimeruyo!", getpid());
            // phone serverと通話する
            call(s_to_PS);
        }
    }

    // ここからはphone serverとしての役割にシフトする
    write_to_log("log.txt", "[phone server] PS ni narimashita!", getpid());

    // この後入ってきたクライアント(phone client: PC)と通話するソケットを作る
    int socket_to_PC = socket(PF_INET, SOCK_STREAM,0);

    while(1){
        write_to_log("log.txt", "[phone server] client matteruyo!", getpid());
        check_stopper(100);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(USER_PORT);
        addr.sin_addr.s_addr = INADDR_ANY;

        // socket_to_PCを有効化する（ポート番号を変える必要があるかも？）
        bind(socket_to_PC, (struct sockaddr *)&addr, sizeof(addr));

        // PCを待ち受ける
        listen(socket_to_PC,10);

        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);

        // 
        int s = accept(socket_to_PC, (struct sockaddr *)&client_addr, &len);
        write_to_log("log.txt", "[phone server] client kiya!! \nima kara PS toshite fork suru yo!", getpid());
        int pid = fork();
        if(pid == -1){
            perror("fork");
            exit(1);
        }
        check_stopper(101);
        if(pid == 0){
            write_to_log("log.txt", "[phone server] Server toshite call suru yo", getpid());
            call(s);
            check_stopper(6);
        }
        check_stopper(102);
        close(s);
    }
    write_to_log("log.txt", "[phone server] PS no while nuketa yo!", getpid());
    free(IP_addr);
    check_stopper(7);

    return 0;
}