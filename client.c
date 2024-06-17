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

void call(int s){
    FILE* fp;
    FILE* fp2;
    int num = 4096;
    char data[num];
    char data_send[num];

    char* cmd = "rec -t raw -b 16 -c 1 -e s -r 48000 -";
    fp = popen(cmd,"r");
    char* cmd2 = "play -t raw -b 16 -c 1 -e s -r 48000 -";
    fp2 = popen(cmd2,"w");

    while(1){
        int read_data = fread(data_send,1,num,fp);
        if(read_data == -1){
            perror("read_data");
            exit(1);
        }else {
            if(read_data == 0){
                break;
            }
            else{
                send(s,data_send,num,0);
            }
        }

        int n = recv(s,data,num,0);
        if(n == -1){perror("recv");exit(1);}
        else{
            if(n == 0){break;}
            else{
                int x = fwrite(data,1,num,fp2);
            }
        }
    }
    //end test
}

void lntrim(char *str) {
  char *p;
  p = strchr(str, '\n');
  if(p != NULL) {
    *p = '\0';
  }
}

int main(int argc,char** argv){
    FILE * IP = fopen("IP.txt","w");

    int s = socket(PF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));
    int ret = connect(s,(struct sockaddr *)&addr, sizeof(addr));

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


    char* IP_addr;
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
            addr.sin_port = htons(50000);
            int ret = connect(s,(struct sockaddr *)&addr, sizeof(addr));

            call(s);
        }
    }


    FILE *f = fopen("./log_queue.txt", "a");
    fprintf(f, "dekita \n");
    int ss = socket(PF_INET, SOCK_STREAM,0);
    int a = 50000;
    int i = 0;

    while(1){
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(a);
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
        }

    }
}
