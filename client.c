#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_ROW_LENGTH 4096
#define SERV_PORT 3000

int globalTime = 1;

int main() {
    FILE *reqs = fopen("input.csv", "r");
    if (reqs == NULL) {
        perror("Error opening file");
        return 0;
    }
    int sockfd;
    struct sockaddr_in servaddr;
    char sendline[MAX_ROW_LENGTH], recvline[MAX_ROW_LENGTH];
    char Server_IP[10] = "127.0.0.1";

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Problem in creating the socket");
        exit(6);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(Server_IP);
    servaddr.sin_port = htons(SERV_PORT);
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Problem in connecting to the server");
        exit(3);
    }
    char time[10]; // to synchronize the clinet and server.

    char req[MAX_ROW_LENGTH];
    fgets(req, MAX_ROW_LENGTH, reqs);
    
    while (globalTime < 20) { // Asssuming Sport's Complex is open for 20 mins. 
    // Each iterartion is a minute.
        char temp[4];
        snprintf(temp, sizeof(temp), "%d", globalTime);
        strcpy(time,"time,");
        strcat(time, temp);
        strcpy(sendline, time);
        send(sockfd, sendline, strlen(sendline), 0); // We are okay with blocking as first time should be notified.
        
        sleep(1);
        fseek(reqs, 0, SEEK_SET); // Reset file pointer to the beginning of the file
        while (fgets(req, MAX_ROW_LENGTH, reqs) != NULL) {
            char req_cpy[MAX_ROW_LENGTH];
            strcpy(req_cpy, req);
            int pid, arr_time;
            char* sex;
            char* preference;
            pid = atoi(strtok(req, ","));
            arr_time = atoi(strtok(NULL, ","));
            sex = strtok(NULL, ",");
            preference = strtok(NULL, ",");
            if (arr_time == globalTime){
                pid_t pid = fork();
                if (pid == -1) {
                    perror("Fork failed");
                    exit(0);
                } else if (pid == 0) { 
                    int sockfd;
                    struct sockaddr_in servaddr;
                    char sendline[MAX_ROW_LENGTH], recvline[MAX_ROW_LENGTH];
                    char Server_IP[10] = "127.0.0.1";

                    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        perror("Problem in creating the socket");
                        exit(6);
                    }

                    memset(&servaddr, 0, sizeof(servaddr));
                    servaddr.sin_family = AF_INET;
                    servaddr.sin_addr.s_addr = inet_addr(Server_IP);
                    servaddr.sin_port = htons(SERV_PORT);
                    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
                        perror("Problem in connecting to the server");
                        exit(3);
                    }
                    strcpy(sendline, req_cpy);
                    printf("Request: %s Global Time: %d\n", req_cpy, globalTime);
                    send(sockfd, sendline, strlen(sendline), 0);
                    recv(sockfd, recvline, MAX_ROW_LENGTH, 0);
                    printf("\nString received from the server: %s\n", recvline);
                    printf("-----------------------------------\n");
                    close(sockfd);
                    exit(0);  
                }          
            }
            if (arr_time > globalTime){
                break;
            }
        }
        globalTime++;
    }
    printf("Parent Finished\n");
    wait(NULL);
    printf("Parent Done waiting\n");
    close(sockfd);
    fclose(reqs);
    return 0;
}
