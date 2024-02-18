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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>  

#define MAX_ROW_LENGTH 5120
#define SERV_PORT 3000
#define SERV_PORT2 3010
#define TIMEOUT_SECONDS 30

typedef struct Time {
    int time;
} Time;

// int globalTime = 1;

int main() {

    /* Initializing the shared memory construct to handle IPC. */
    int shmid;
    Time* globalTime;
    shmid = shmget(IPC_PRIVATE, sizeof(Time), IPC_CREAT | 0666);
        if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    /* Attach the shared memory segment. */
    globalTime = (Time *)shmat(shmid, NULL, 0);
    if (globalTime == (Time *)-1) {
        perror("shmat");
        return 1;
    }

    FILE *reqs = fopen("input.csv", "r");
    if (reqs == NULL) {
        perror("Error opening file");
        return 0;
    }
    int sockfd, sockfd2;
    struct sockaddr_in servaddr;
    char sendline[MAX_ROW_LENGTH], recvline[MAX_ROW_LENGTH];
    char Server_IP[10] = "127.0.0.1";

    char req[MAX_ROW_LENGTH];
    fgets(req, MAX_ROW_LENGTH, reqs); // Remove first header in csv.
    
   while (globalTime->time < 25) { // Assuming Sport's Complex is open for 50 mins. 
        fseek(reqs, 0, SEEK_SET); // Reset file pointer to the beginning of the file
        while (fgets(req, MAX_ROW_LENGTH, reqs) != NULL) {
            // printf("Request: %s", req);

            // Check for empty line or invalid input
            if (req[0] == '\n' || req[0] == '\0') {
                // printf("Empty line or invalid input\n");
                continue;
            }

            char req_cpy[MAX_ROW_LENGTH];
            strcpy(req_cpy, req);
            int plid, arr_time;
            char* sex;
            char* preference;
            char* token = strtok(req, ",");
            if (token == NULL) {
                // printf("Invalid input format\n");
                continue;
            }
            plid = atoi(token);
            token = strtok(NULL, ",");
            if (token == NULL) {
                // printf("Invalid input format\n");
                continue;
            }
            arr_time = atoi(token);
            token = strtok(NULL, ",");
            if (token == NULL) {
                // printf("Invalid input format\n");
                continue;
            }
            sex = token;
            token = strtok(NULL, ",");
            if (token == NULL) {
                // printf("Invalid input format\n");
                continue;
            }
            preference = token;

            if (arr_time == globalTime->time) {
                pid_t pid = fork();
                if (pid == -1) {
                    perror("Fork failed");
                    exit(1);
                } else if (pid == 0) {
                    // Child process
                    // Perform child process tasks here
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
                        perror("Problem in connecting to the server scheduler");
                        exit(3);
                    }

                    
                    strcpy(sendline, req_cpy);
                    printf("Request: %s\n", req_cpy);
                    send(sockfd, sendline, strlen(sendline), 0);
                    
                    int flags = fcntl(sockfd, F_GETFL, 0);
                    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
                    int timeout = TIMEOUT_SECONDS;
                    int n;

                    while (timeout > 0) {                       
                        n = recv(sockfd, recvline, MAX_ROW_LENGTH, 0);

                        // Check if data received
                        if (n > 0) {
                            close(sockfd);
                            break;
                        } else if (n == 0) {
                            printf("Connection closed by the peer\n");
                            break;
                        } else {
                            timeout--;
                            sleep(1); // Sleep for 1 s
                        }
                    }
                    if (timeout == 0) {
                        printf("Player %d Timedout at %d\n",plid,globalTime->time);
                        close(sockfd);
                        exit(0);
                        break;
                    }

                    // int n = recv(sockfd, recvline, MAX_ROW_LENGTH, 0);
                    // close(sockfd);
                    // return courtid,starttime,endtime,num_players,playerIDs,caller
                    // printf("Player: %d Recieved %s\n",plid,recvline);

                    char gamesched[n];
                    strcpy(gamesched,recvline);

                    // printf("gamesched: %s",gamesched);

                    int courtid, start_time, end_time, num_players;
                    courtid = atoi(strtok(gamesched, ","));
                    start_time = atoi(strtok(NULL, ","));
                    end_time = atoi(strtok(NULL, ","));
                    num_players = atoi(strtok(NULL, ","));
                    int players[num_players];
                    for (int i = 0; i < num_players; i++) {
                        players[i] = atoi(strtok(NULL, ","));
                    }
                    int caller = atoi(strtok(NULL, ","));
                    if ((sockfd2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        perror("Problem in creating the socket");
                        exit(6);
                    }

                    memset(&servaddr, 0, sizeof(servaddr));
                    servaddr.sin_family = AF_INET;
                    servaddr.sin_addr.s_addr = inet_addr(Server_IP);
                    servaddr.sin_port = htons(SERV_PORT2);
                    if (connect(sockfd2, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
                        perror("Problem in connecting to the server game play");
                        exit(3);
                    }
                    while(1){
                        if (start_time == globalTime->time){
                            char msg[20];
                            int len = sprintf(msg,"%d\n",end_time);
                            send(sockfd2, msg, 20, 0);
                            break;
                        }
                    }
                    char game_finish[20];
                    // printf("Hello! %d\n",plid);
                    recv(sockfd2, game_finish, 20, 0);
                    printf("\nGame Finished by %d at %swhen client time is: %d\n", plid, game_finish, globalTime->time);
                    printf("-----------------------------------\n");
                    close(sockfd2);
                    exit(0);  
                }
            } else if (arr_time > globalTime->time) {
                break;
            }
        }
        sleep(1);
        globalTime->time++;
    }

    fclose(reqs);
    return 0;
}



          // int sockfd;
                    // struct sockaddr_in servaddr;
                    // char sendline[MAX_ROW_LENGTH], recvline[MAX_ROW_LENGTH];
                    // char Server_IP[10] = "127.0.0.1";

                    // if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    //     perror("Problem in creating the socket");
                    //     exit(6);
                    // }

                    // memset(&servaddr, 0, sizeof(servaddr));
                    // servaddr.sin_family = AF_INET;
                    // servaddr.sin_addr.s_addr = inet_addr(Server_IP);
                    // servaddr.sin_port = htons(SERV_PORT);
                    // if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
                    //     perror("Problem in connecting to the server scheduler");
                    //     exit(3);
                    // }
                    // strcpy(sendline, req_cpy);
                    // printf("Request: %s\n", req_cpy);
                    // send(sockfd, sendline, strlen(sendline), 0);
                    // int n = recv(sockfd, recvline, MAX_ROW_LENGTH, 0);
                    // // return courtid,starttime,endtime,num_players,playerIDs,caller
                    // printf("Player: %d Recieved %s\n",plid,recvline);

                    // char gamesched[n];
                    // strcpy(gamesched,recvline);

                    // printf("gamesched: %s",gamesched);

                    // int courtid, start_time, end_time, num_players;
                    // courtid = atoi(strtok(gamesched, ","));
                    // start_time = atoi(strtok(NULL, ","));
                    // end_time = atoi(strtok(NULL, ","));
                    // num_players = atoi(strtok(NULL, ","));
                    // int players[num_players];
                    // for (int i = 0; i < num_players; i++) {
                    //     players[i] = atoi(strtok(NULL, ","));
                    // }
                    // int caller = atoi(strtok(NULL, ","));
                    // if ((sockfd2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    //     perror("Problem in creating the socket");
                    //     exit(6);
                    // }

                    // memset(&servaddr, 0, sizeof(servaddr));
                    // servaddr.sin_family = AF_INET;
                    // servaddr.sin_addr.s_addr = inet_addr(Server_IP);
                    // servaddr.sin_port = htons(SERV_PORT2);
                    // if (connect(sockfd2, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
                    //     perror("Problem in connecting to the server game play");
                    //     exit(3);
                    // }
                    // while(1){
                    //     if (start_time == globalTime->time){
                    //         char msg[20];
                    //         int len = sprintf(msg,"%d\n",end_time);
                    //         send(sockfd2, msg, 20, 0);
                    //         break;
                    //     }
                    // }
                    // char game_finish[20];
                    // printf("Hello! %d\n",plid);
                    // recv(sockfd2, game_finish, 20, 0);
                    // printf("\nGame Finished by %d at %swhen client time is: %d\n", plid, game_finish, globalTime->time);
                    // printf("-----------------------------------\n");
                    // close(sockfd);
                    // close(sockfd2);
                    // exit(0);  