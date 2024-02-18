#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <omp.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define MAXLINE 5120 /*max text line length*/
#define SERV_PORT 3000 /*port*/
#define SERV_PORT2 3010
#define LISTENQ 10 /*maximum number of client connections*/
#define MAX_PLAYERS 300 // 5*60
#define MAX_COURTS 4
#define MAX_ROW_LENGTH 5120
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MAX_PLAYER_ID_LENGTH 3

int main() {
    int listenfd2, connfd2, n;
    socklen_t clilen2;
    char buf[MAXLINE];
    char gamereq[3];
    struct sockaddr_in cliaddr2, servaddr2;

    listenfd2 = socket (AF_INET, SOCK_STREAM, 0);

    servaddr2.sin_family = AF_INET;
    servaddr2.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr2.sin_port = htons(SERV_PORT2);
        
    bind(listenfd2, (struct sockaddr *) &servaddr2, sizeof(servaddr2)); 
    listen(listenfd2, LISTENQ);
        
    printf("%s\n","Server running...waiting for connections.");
    #pragma omp parallel
    {   
        #pragma omp single 
        {
            for( ; ; ) {
                clilen2 = sizeof(cliaddr2);
                connfd2 = accept(listenfd2, (struct sockaddr *) &cliaddr2, &clilen2);
                if (connfd2 < 0) {
                    perror("Problem in accepting connection");
                    exit(3);
                }

                #pragma omp task firstprivate(connfd2)
                {
                    while ((n = recv(connfd2, gamereq, MAXLINE, 0)) > 0) {
                        printf("Recieved: %s\n",gamereq);
                        int counter = atoi(gamereq);
                        int time = counter;
                        counter -= 1;
                        while (counter > 0){
                            sleep(1);
                            counter--;
                        }
                        char reply[20];
                        sprintf(reply,"%d\n",time);
                        send(connfd2, reply, 20, 0);
                        close(connfd2);
                    }
                }
            }
        }
    }
    close(listenfd2);
    return 0;
}
