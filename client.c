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
#define TIMEOUT_SECONDS 5

typedef struct Time {
    int time;
} Time;

int main(int argc, char** argv) {

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

    FILE *reqs = fopen(argv[1], "r");
    if (reqs == NULL) {
        perror("Error opening file");
        return 0;
    }
    int sockfd;
    struct sockaddr_in servaddr;
    char sendline[MAX_ROW_LENGTH], recvline[MAX_ROW_LENGTH];
    char Server_IP[10] = "127.0.0.1";

    char req[MAX_ROW_LENGTH];
    fgets(req, MAX_ROW_LENGTH, reqs); // Remove first header in csv.
    
   while (globalTime->time < 100) { // Assuming Sport's Complex is open for 100 mins. 
        fseek(reqs, 0, SEEK_SET); // Reset file pointer to the beginning of the file
        fgets(req, MAX_ROW_LENGTH, reqs);
        while (fgets(req, MAX_ROW_LENGTH, reqs) != NULL) {
            if (req[0] == '\n' || req[0] == '\0') {
                continue;
            }

            char req_cpy[MAX_ROW_LENGTH];
            strcpy(req_cpy, req);
            int plid, arr_time;
            char* sex;
            char* preference;
            char* token = strtok(req, ",");
            if (token == NULL) {
                continue;
            }
            plid = atoi(token);
            token = strtok(NULL, ",");
            if (token == NULL) {
                continue;
            }
            arr_time = atoi(token);
            token = strtok(NULL, ",");
            if (token == NULL) {
                continue;
            }
            sex = token;
            token = strtok(NULL, ",");
            if (token == NULL) {
                continue;
            }
            preference = token;

            if (arr_time == globalTime->time) {
                pid_t pid = fork();
                if (pid == -1) {
                    perror("Fork failed");
                    exit(1);
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
                        perror("Problem in connecting to the server scheduler");
                        exit(3);
                    }

                    
                    strcpy(sendline, req_cpy);
                    printf("PLID %d Requests: %s\n", plid, req_cpy);
                    send(sockfd, sendline, strlen(sendline), 0);
                    
                    int flags = fcntl(sockfd, F_GETFL, 0);
                    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
                    int timeout = TIMEOUT_SECONDS;
                    int n;

                    while (timeout > 0) {                      
                        n = recv(sockfd, recvline, MAX_ROW_LENGTH, 0);
                        if (n > 0) {
                            printf("PLID %d says Data Recieved\n",plid);
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

                    char gamesched[n];
                    strcpy(gamesched,recvline);

                    int courtid, start_time, end_time, num_players;
                    courtid = atoi(strtok(gamesched, ","));
                    start_time = atoi(strtok(NULL, ","));
                    end_time = atoi(strtok(NULL, ","));
                    num_players = atoi(strtok(NULL, ","));
                    int players[num_players];
                    for (int i = 0; i < num_players; i++) {
                        players[i] = atoi(strtok(NULL, ","));
                    }
                    // int caller = atoi(strtok(NULL, ","));
                    while(1){
                        if (start_time + 1 == globalTime->time){ // Adding 2 second delay to allow for transmission (communication) delays
                            printf("PLID %d, Going to play tennis at client time %d\n", plid, globalTime->time);
                            sleep(end_time - start_time);
                            break;
                        }
                    }

                    printf("PLID %d, Waiting to return\n",plid);
                    printf("\nGame Finished by %d when client time is: %d\n", plid, globalTime->time);
                    printf("-----------------------------------\n");
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