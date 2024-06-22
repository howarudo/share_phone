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
#include <time.h>

 
#define  MAX_LEN  100
 
int main(void)
{
   FILE *fp;
   char line[MAX_LEN], *result;
 
   fp = fopen("myIP.txt","r");
 
   while (fgets(line,20,fp) != NULL){
      printf("The string is [%s]\n", line);
   
   int k = fork();
   if (k==0){
    while(1){
        sleep(100);
    }
   }
   }
   if (fclose(fp))
      perror("fclose error");
}