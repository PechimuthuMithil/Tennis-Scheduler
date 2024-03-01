// -np x+1 where x is the maximum possible clients we will ever see in a day.

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
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
#include <string.h>

#define MAX_ROW_LENGTH 5120
#define SERV_PORT 3000
#define SERV_PORT2 3010
#define TIMEOUT_SECONDS 5

typedef struct Time {
    int time;
} Time;

int main(int argc, char** argv) {

    int sockfd, sockfd2;
    struct sockaddr_in servaddr;
    char sendline[MAX_ROW_LENGTH], recvline[MAX_ROW_LENGTH];
    char Server_IP[10] = "127.0.0.1";
    MPI_Init(argc, argv);

    MPI_Win win;
    Time* globalTime;
    MPI_Aint size;
    int disp_unit;

    MPI_Win_allocate_shared(sizeof(Time), sizeof(Time), MPI_INFO_NULL, MPI_COMM_WORLD, &globalTime, &win);

    MPI_Win_shared_query(win, MPI_PROC_NULL, &size, &disp_unit, &globalTime); // Pass address of disp_unit
    
    int world_rank;
     MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

if (world_rank == 0) {
    globalTime->time = 1;
    FILE *reqs = fopen(argv[1], "r");
    if (reqs == NULL) {
        perror("Error opening file");
        return 0;
    }

    char req[MAX_ROW_LENGTH];
    fgets(req, MAX_ROW_LENGTH, reqs); // Remove first header in csv.
    while (globalTime->time < 50) {
        fseek(reqs, 0, SEEK_SET); // Reset file pointer to the beginning of the file
        fgets(req, MAX_ROW_LENGTH, reqs);
        while (fgets(req, MAX_ROW_LENGTH, reqs) != NULL) {
            if (req[0] == '\n' || req[0] == '\0') {;
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
                    MPI_Send(
                  /* data         = */ &req_cpy, 
                  /* count        = */ strlen(req_cpy), 
                  /* datatype     = */ MPI_CHAR, 
                  /* destination  = */ plid, 
                  /* tag          = */ 0, 
                  /* communicator = */ MPI_COMM_WORLD);
            }
            else if (arr_time > globalTime->time) {
                break;
            }
        }
        sleep(1);
        globalTime->time ++;
    }
    fclose(reqs);
}
    else {
        char req[MAX_ROW_LENGTH];
        MPI_Recv(
          /* data         = */ &req, 
          /* count        = */ MAX_ROW_LENGTH, 
          /* datatype     = */ MPI_CHAR, 
          /* source       = */ 0, 
          /* tag          = */ 0, 
          /* communicator = */ MPI_COMM_WORLD, 
          /* status       = */ MPI_STATUS_IGNORE);

        printf("Rank %d received --> %s\n",world_rank,req);
        char req_cpy[MAX_ROW_LENGTH];
        strcpy(req_cpy, req);
        int plid, arr_time;
        char* sex;
        char* preference;
        char* token = strtok(req, ",");
        plid = atoi(token);
        token = strtok(NULL, ",");
        arr_time = atoi(token);
        token = strtok(NULL, ",");
        sex = token;
        token = strtok(NULL, ",");
        preference = token;

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
            // Check if data received
            if (n > 1) {
                printf("PLID %d says Data Recieved: %s\n",plid, recvline);
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
            // break;
        }
        else {
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

            while(1){
                if (start_time+1 == globalTime->time){ // Adding 1 second delay to allow for transmission (communication) delays
                    printf("PLID %d, Going to play tennis at client time %d\n", plid, globalTime->time);
                    sleep(end_time - start_time);
                    break;
                }
            }
            printf("\nGame Finished by %d when client time is: %d\n", plid, globalTime->time);
            printf("-----------------------------------\n"); 
            // let's say the caller is the winner. So winner will say. 
            if (num_players == 2){
                int winner = players[0];
            int looser = players[1];

            char greet_w[] = "Well Played! Congratulations on winning!\n";
            char greet_l[] = "Thank you looser!\n";
            char reply[50];
            // Looser greeting the winner.
            if (plid == looser){
                MPI_Send(
                /* data         = */ &greet_w, 
                /* count        = */ strlen(greet_w), 
                /* datatype     = */ MPI_CHAR, 
                /* destination  = */ winner, 
                /* tag          = */ 0, 
                /* communicator = */ MPI_COMM_WORLD);

                MPI_Recv(
                /* data         = */ &reply, 
                /* count        = */ 50, 
                /* datatype     = */ MPI_CHAR, 
                /* source       = */ winner, 
                /* tag          = */ 0, 
                /* communicator = */ MPI_COMM_WORLD, 
                /* status       = */ MPI_STATUS_IGNORE);

                printf("%d, %d To %d, %d: %s",winner,winner,looser,looser,reply);
            }
            else if (plid == winner){
                MPI_Recv(
                /* data         = */ &reply, 
                /* count        = */ 50, 
                /* datatype     = */ MPI_CHAR, 
                /* source       = */ looser, 
                /* tag          = */ 0, 
                /* communicator = */ MPI_COMM_WORLD, 
                /* status       = */ MPI_STATUS_IGNORE);

                printf("%d, %d To %d, %d: %s",looser,looser,winner,winner,reply);

                MPI_Send(
                /* data         = */ &greet_l, 
                /* count        = */ strlen(greet_l), 
                /* datatype     = */ MPI_CHAR, 
                /* destination  = */ looser, 
                /* tag          = */ 0, 
                /* communicator = */ MPI_COMM_WORLD);

            }
            }
            else {
                int winner1 = players[0];
                int winner2 = players[1];
                int looser1 = players[2];
                int looser2 = players[3];
                char greet_w[] = "Well Played! Congratulations on winning!\n";
                char greet_l[] = "Thank you looser!\n";
                char reply[50];
                // Looser greeting the winner.
                if (plid == looser1 || plid == looser2){
                    MPI_Send(
                    /* data         = */ &greet_w, 
                    /* count        = */ strlen(greet_w), 
                    /* datatype     = */ MPI_CHAR, 
                    /* destination  = */ winner1, 
                    /* tag          = */ 0, 
                    /* communicator = */ MPI_COMM_WORLD);

                    MPI_Recv(
                    /* data         = */ &reply, 
                    /* count        = */ 50, 
                    /* datatype     = */ MPI_CHAR, 
                    /* source       = */ winner1, 
                    /* tag          = */ 0, 
                    /* communicator = */ MPI_COMM_WORLD, 
                    /* status       = */ MPI_STATUS_IGNORE);

                    printf("%d, %d To %d, %d: %s",winner1,winner1,plid,plid,reply);
                    
                    MPI_Send(
                    /* data         = */ &greet_w, 
                    /* count        = */ strlen(greet_w), 
                    /* datatype     = */ MPI_CHAR, 
                    /* destination  = */ winner2, 
                    /* tag          = */ 0, 
                    /* communicator = */ MPI_COMM_WORLD);

                    MPI_Recv(
                    /* data         = */ &reply, 
                    /* count        = */ 50, 
                    /* datatype     = */ MPI_CHAR, 
                    /* source       = */ winner2, 
                    /* tag          = */ 0, 
                    /* communicator = */ MPI_COMM_WORLD, 
                    /* status       = */ MPI_STATUS_IGNORE);

                    printf("%d, %d To %d, %d: %s",winner2,winner2,plid,plid,reply);
                }
                else if (plid == winner1 || plid == winner2){
                    MPI_Recv(
                    /* data         = */ &reply, 
                    /* count        = */ 50, 
                    /* datatype     = */ MPI_CHAR, 
                    /* source       = */ looser1, 
                    /* tag          = */ 0, 
                    /* communicator = */ MPI_COMM_WORLD, 
                    /* status       = */ MPI_STATUS_IGNORE);

                    printf("%d, %d To %d, %d: %s",looser1,looser1,plid,plid,reply);

                    MPI_Send(
                    /* data         = */ &greet_l, 
                    /* count        = */ strlen(greet_l), 
                    /* datatype     = */ MPI_CHAR, 
                    /* destination  = */ looser1, 
                    /* tag          = */ 0, 
                    /* communicator = */ MPI_COMM_WORLD);

                    MPI_Recv(
                    /* data         = */ &reply, 
                    /* count        = */ 50, 
                    /* datatype     = */ MPI_CHAR, 
                    /* source       = */ looser2, 
                    /* tag          = */ 0, 
                    /* communicator = */ MPI_COMM_WORLD, 
                    /* status       = */ MPI_STATUS_IGNORE);

                    printf("%d, %d To %d, %d: %s",looser2,looser2,plid,plid,reply);

                    MPI_Send(
                    /* data         = */ &greet_l, 
                    /* count        = */ strlen(greet_l), 
                    /* datatype     = */ MPI_CHAR, 
                    /* destination  = */ looser2, 
                    /* tag          = */ 0, 
                    /* communicator = */ MPI_COMM_WORLD);

                }
            }
        }
    }
    MPI_Win_free(&win);
  MPI_Finalize();
}