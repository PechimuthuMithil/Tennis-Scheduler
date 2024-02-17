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

typedef struct PlayerNode {
    int player_id;
    int connfd;
    int arrival_time;
    char gender;
    char preference;
    struct PlayerNode* next;
    struct PlayerNode* prev;
} PlayerNode;

typedef struct {
    PlayerNode* front;
    PlayerNode* rear;
    int count;
} Queue;

typedef struct {
    int court_number;
    int endtime;
    int game_start_time;
    int player_ids[4];
} Court;

typedef struct {
    Queue* singles_queue;
    Queue* doubles_queue;
} Pointers;

Queue singles_queue, doubles_queue;
Court courts[MAX_COURTS];
char outputf[10];
bool singles_queue_empty = true;
bool doubles_queue_empty = true;
bool enquing_done = false;

// void reader_function();
void singles_function();
void doubles_function();
void enqueue(Queue* queue, int player_id, int arrival_time, char gender, char preference, int connfd, bool* empty);
void dequeue(Queue* queue, bool* empty);
void schedule_game(Queue* queue, int court_number, int numplayers);
void enq_front(Queue* queue, int player_id, int arrival_time, char gender, char preference, bool* empty);

void singles_function() {
    int min_endtime_court = 0;
    while (1) {
        if (singles_queue.count < 2/*singles_queue.front == NULL || singles_queue.front->next == NULL*/) {
            if (enquing_done){
                break;
            }
            continue;
        }
        // sem_wait(&search);
        #pragma omp critical
        {
        for (int i = 0; i < MAX_COURTS; i++) {
            if (courts[i].endtime < courts[min_endtime_court].endtime) {
                min_endtime_court = i;
            }
        }
        schedule_game(&singles_queue, min_endtime_court, 2); // Schedule singles game on court i
        dequeue(&singles_queue, &singles_queue_empty);
        dequeue(&singles_queue, &singles_queue_empty);
        }
        // sem_post(&search); // Release lock on the court
        if (enquing_done && singles_queue_empty){
            break;
        }
    }
    // return NULL;
}

void doubles_function() {
    int min_endtime_court = 0;
    while (1) {
        if (doubles_queue.count < 4/*singles_queue.front == NULL || singles_queue.front->next == NULL*/) {
            if (enquing_done){
                break;
            }
            continue;
        }
        // sem_wait(&search);
        #pragma omp critical
        {
        for (int i = 0; i < MAX_COURTS; i++) {
            if (courts[i].endtime < courts[min_endtime_court].endtime) {
                min_endtime_court = i;
            }
        }
        schedule_game(&doubles_queue, min_endtime_court, 4); // Schedule singles game on court i
        dequeue(&doubles_queue, &doubles_queue_empty);
        dequeue(&doubles_queue, &doubles_queue_empty);
        dequeue(&doubles_queue, &doubles_queue_empty);
        dequeue(&doubles_queue, &doubles_queue_empty);
        }
        // sem_post(&search); // Release lock on the court
        if (enquing_done && doubles_queue_empty){
            break;
        }
    }
    // return NULL;
}

void enqueue(Queue* queue, int player_id, int arrival_time, char gender, char preference, int connfd, bool* empty) {
    PlayerNode* newNode = (PlayerNode*)malloc(sizeof(PlayerNode));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->player_id = player_id;
    newNode->connfd = connfd;
    newNode->arrival_time = arrival_time;
    newNode->gender = gender;
    newNode->preference = preference;
    newNode->next = NULL;
    newNode->prev = queue->rear;

    if (queue->rear == NULL) {
        queue->front = newNode;
    } else {
        queue->rear->next = newNode;
    }
    queue->rear = newNode;
    queue->count++;
    *empty = false;
}

void dequeue(Queue* queue, bool* empty) {
    if (queue->front == NULL) {
        fprintf(stderr, "Queue is empty\n");
        return;
    }
    PlayerNode* temp = queue->front;
    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    } else {
        queue->front->prev = NULL;
    }
    free(temp);
    queue->count--;
    if (queue->count == 0){
        *empty = true;
    }
}

void schedule_game(Queue* queue, int court_number, int numplayers) {
    int endtime;

    // Calculate end time based on the number of male players and game type
    int num_male_players = 0;
    PlayerNode* current = queue->front;
    for (int i = 0; i < numplayers; i++) {
        if (current->gender == 'M') {
            num_male_players++;
        }
        current = current->next;
    }

    if (num_male_players == 0) {
        endtime = (numplayers == 2) ? 5 : 10; 
    } else {
        endtime = (numplayers == 2) ? 10 : 15; 
    }

    if (numplayers == 2){
        courts[court_number].game_start_time = MAX(queue->front->next->arrival_time, courts[court_number].endtime);
    }
    else {
        courts[court_number].game_start_time = MAX(queue->front->next->next->next->arrival_time, courts[court_number].endtime);
    }
    endtime += courts[court_number].game_start_time;
    courts[court_number].endtime = endtime; 
    // return courtid,starttime,endtime,num_players,playerIDs,caller
    char msg[MAX_ROW_LENGTH];
   
    int chars_written = sprintf(msg,"%d,%d,%d,%d,",courts[court_number].court_number,courts[court_number].game_start_time,courts[court_number].endtime,numplayers);
    // int chars_written = sprintf(msg1, "Game-start-time: %d, End time: %d, Court-Number: %d, List-of-player-ids: ", 
                                // courts[court_number].game_start_time, courts[court_number].endtime, courts[court_number].court_number);
    // printf("Game-start-time: %d, End time: %d, Court-Number: %d, List-of-player-ids: ", 
        //    courts[court_number].game_start_time, courts[court_number].endtime, courts[court_number].court_number);
    current = queue->front;
    for (int i = 0; i < numplayers; i++) {
        char msg2[3];
        chars_written += sprintf(msg2,"%d,",current->player_id);
        strcat(msg,msg2);
        current = current->next;
    }
    char msg2[3];
    chars_written += sprintf(msg2,"%d\n",queue->front->player_id);
    strcat(msg,msg2);

    current = queue->front;
    // FILE* output = fopen("output.csv", "a");
    // fprintf(output, "%d,%d,%d", courts[court_number].game_start_time, courts[court_number].endtime, courts[court_number].court_number);
    for (int i = 0; i < numplayers; i++) {
        // printf("%d ",current->player_id);
        // fprintf(output,",%d", current->player_id);
        send(current->connfd, msg, chars_written, 0);
        close(current->connfd);
        printf("Sending %d --> %s\n",current->connfd, msg);
        current = current->next;
    }
    // fprintf(output, "\n");
    // fclose(output);
    // printf("\n");
}

int main() {
    int listenfd, listenfd2, connfd, connfd2, n;
    socklen_t clilen, clilen2;
    char buf[MAXLINE];
    char gamereq[3];
    struct sockaddr_in cliaddr, cliaddr2, servaddr, servaddr2;

    //creation of the socket
    listenfd = socket (AF_INET, SOCK_STREAM, 0);
    listenfd2 = socket (AF_INET, SOCK_STREAM, 0);

    //preparation of the socket address 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    servaddr2.sin_family = AF_INET;
    servaddr2.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr2.sin_port = htons(SERV_PORT2);
        
    bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    bind(listenfd2, (struct sockaddr *) &servaddr2, sizeof(servaddr2)); 
    listen(listenfd, LISTENQ);
    listen(listenfd2, LISTENQ);
        
    // printf("%s\n","Server running...waiting for connections.");

    singles_queue.front = singles_queue.rear = NULL;
    singles_queue.count = 0;
    doubles_queue.front = doubles_queue.rear = NULL;
    doubles_queue.count = 0;

    for (int i = 0; i < MAX_COURTS; i++) {
        courts[i].court_number = i + 1;
        courts[i].game_start_time = 0;
        courts[i].endtime = 0;

    }
    // sem_init(&search,0,1);

    FILE* output = fopen("output.csv", "w+");
    fprintf(output, "Game-start-time,Game-end-time,Court-Number,List-of-player-ids\n");
    fclose(output);

    printf("%s\n","Server running...waiting for connections.");
    #pragma omp parallel
    {   
        #pragma omp single 
        {
            #pragma omp task
            singles_function();
            #pragma omp task 
            doubles_function();
    
            for (;;) {
                clilen = sizeof(cliaddr);
                connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
                if (connfd < 0) {
                    perror("Problem in accepting connection");
                    exit(3);
                }
                // printf("%s\n", "Received request...");
                #pragma omp task firstprivate(connfd)
                {
                    while ((n = recv(connfd, buf, MAXLINE, 0)) > 0) {
                        char req[MAXLINE];
                        // printf("%s\n",buf);
                        strcpy(req, buf);
                        int pid, arr_time;
                        char* sex;
                        char* preference;
                        pid = atoi(strtok(req, ","));
                        arr_time = atoi(strtok(NULL, ","));
                        sex = strtok(NULL, ",");
                        preference = strtok(NULL, ",");
                        
                        if (sex == NULL || preference == NULL) {
                            fprintf(stderr, "Invalid player data in request\n");
                            continue; 
                        }
                        size_t len_sex = strlen(sex);
                        if (len_sex > 0 && sex[len_sex - 1] == '\n') {
                            sex[len_sex - 1] = '\0';
                        }
                        size_t len_pref = strlen(preference);
                        if (len_pref > 0 && preference[len_pref - 1] == '\n') {
                            preference[len_pref - 1] = '\0';
                        }

                        if (preference[0] == 'S' || preference[0] == 'b') {
                            #pragma omp critical 
                            {
                                // printf("Single count: %d\n", singles_queue.count);
                                enqueue(&singles_queue, pid, arr_time, sex[0], preference[0], connfd, &singles_queue_empty);
                                // printf("Single count: %d\n", singles_queue.count);
                            }
                        }
                        else if (preference[0] == 'D' || preference[0] == 'B') {
                            #pragma omp critical
                            {
                                // printf("Double count: %d\n", doubles_queue.count);
                                enqueue(&doubles_queue, pid, arr_time, sex[0], preference[0], connfd, &doubles_queue_empty);
                                // printf("Double count: %d\n", doubles_queue.count);
                            }
                        }
                        else {
                            fprintf(stderr, "Invalid preference for player %d\n", pid);
                        }
                        // send(connfd, buf, n, 0);
                    }

                    if (n < 0) 
                        printf("%s\n", "Read error");
                    
                    // close(connfd);
                }
            }
        }
    }
    close(listenfd); 
    close(listenfd2);
    return 0;
}
