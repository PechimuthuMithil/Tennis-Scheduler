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

#define MAXLINE 5120 /*max text line length*/
#define SERV_PORT 3000 /*port*/
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
    int end_time;
    char gender;
    char preference;
    struct PlayerNode* next;
    struct PlayerNode* prev;
} PlayerNode;


typedef struct Games{
    int court_number;
    int start_time;
    int end_time;
    int num_players;
    int players[4];
    struct Games* next;
    struct Games* prev;
} Games;

typedef struct {
    Games* front;
    Games* rear;
    int count;
} GameQueue;

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

Queue singles_queue, doubles_queue;
GameQueue final_queue;
Court courts[MAX_COURTS];
char outputf[10];
bool singles_queue_empty = true;
bool doubles_queue_empty = true;
bool enquing_done = false;
int globalTime = 1;

void enqueue_game(GameQueue* queue, int court_number, int start_time, int end_time, int players[4], int num_players);
void dequeue_game(GameQueue* queue);
void singles_function();
void doubles_function();
void doubles_function();
void enqueue(Queue* queue, int player_id, int arrival_time, char gender, char preference, int connfd, bool* empty);
void dequeue(Queue* queue, bool* empty);
void schedule_game(Queue* queue, int court_number, int numplayers);
void handle_request(int connfd);
void final_function();

void enqueue_game(GameQueue* queue, int court_number, int start_time, int end_time, int players[4], int num_players) {
    // Create a new node
    Games* new_node = (Games*)malloc(sizeof(Games));
    if (new_node == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    new_node->court_number = court_number;
    new_node->start_time = start_time;
    new_node->end_time = end_time;
    for (int i = 0; i < num_players; i++) {
        new_node->players[i] = players[i];
    }
    new_node->next = NULL;
    new_node->prev = NULL;

    // If the GameQueue is empty, set both front and rear to the new node
    if (queue->count == 0) {
        queue->front = new_node;
        queue->rear = new_node;
    } else {
        // Otherwise, add the new node to the rear of the GameQueue
        queue->rear->next = new_node;
        new_node->prev = queue->rear;
        queue->rear = new_node;
    }

    // Increment the count of nodes in the GameQueue
    queue->count++;
}

// Function to dequeue a node from the GameQueue
void dequeue_game(GameQueue* queue) {
    if (queue->count == 0) {
        return;
    }

    Games* temp = queue->front;
   
    if (queue->front == queue->rear) {
        queue->front = NULL;
        queue->rear = NULL;
    } else {

        queue->front = queue->front->next;
        queue->front->prev = NULL;
    }

    free(temp);

    queue->count--;
}

void singles_function() {
    int min_endtime_court = 0;
    while (1) {
        if (singles_queue.count < 2/*singles_queue.front == NULL || singles_queue.front->next == NULL*/) {
            if (enquing_done){
                break;
            }
            continue;
        }
        printf("Hello\n");

        for (int i = 0; i < MAX_COURTS; i++) {
            if (courts[i].endtime < courts[min_endtime_court].endtime) {
                min_endtime_court = i;
            }
        }
        printf("Scheduling a game\n");
        schedule_game(&singles_queue, min_endtime_court, 2); // Schedule singles game on court i
        dequeue(&singles_queue, &singles_queue_empty);
        dequeue(&singles_queue, &singles_queue_empty);

        if (enquing_done && singles_queue_empty){
            break;
        }
    }
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

        if (enquing_done && doubles_queue_empty){
            break;
        }
    }
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
    newNode->end_time = arrival_time + 30;
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
    printf("Hi\n");
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
    int players[numplayers];

    // printf("Game-start-time: %d, End time: %d, Court-Number: %d, List-of-player-ids: ", 
    //        courts[court_number].game_start_time, courts[court_number].endtime, courts[court_number].court_number);

    current = queue->front;

    for (int i = 0; i < numplayers; i++) {
        current->end_time = endtime;
        players[i] = current->connfd;
        current = current->next;
    }
    enqueue_game(&final_queue, courts[court_number].court_number, courts[court_number].game_start_time, courts[court_number].endtime, players, numplayers);
    printf("Game enqueued\n");
}

void handle_request(int connfd) {
    char buf[MAXLINE];
    char req[MAXLINE];
    // Receive data from client
    size_t n = recv(connfd, buf, MAXLINE, 0);
    printf("Recieved: %s\n",buf);
    if (n <= 0) {
        perror("Error receiving data from client");
        close(connfd);
        return;
    }
    strcpy(req, buf);
    char* header;
    int time;
    header = strtok(buf, ",");
    time = atoi(strtok(NULL,","));

    if (strcmp(header,"time") == 0){
        #pragma omp critical
        {
        globalTime = time;
        }
        printf("Time: %d\n",globalTime);
        send(connfd, "0", 1, 0);
        close(connfd);
        return;
    }

    int pid, arr_time;
    char* sex;
    char* preference;
    pid = atoi(strtok(req, ","));
    arr_time = atoi(strtok(NULL, ","));
    sex = strtok(NULL, ",");
    preference = strtok(NULL, ",");
    
    if (sex == NULL || preference == NULL) {
        fprintf(stderr, "Invalid player data in CSV\n");
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
        enqueue(&singles_queue, pid, arr_time, sex[0], preference[0], connfd, &singles_queue_empty);
        printf("Enqued into single\n");
    }
    else if (preference[0] == 'D' || preference[0] == 'B') {
        enqueue(&doubles_queue, pid, arr_time, sex[0], preference[0], connfd, &doubles_queue_empty);
    }
    else {
        fprintf(stderr, "Invalid preference for player %d\n", pid);
    }
    return;
}

void final_function(){
    printf("Hi final\n");
    while (1) {
        PlayerNode* ptr;
        ptr = singles_queue.front;
        PlayerNode* next;
        while (ptr) {
            if (globalTime == ptr->end_time){
                char buf[] = "Time out at ";
                char temp[4];
                snprintf(temp, sizeof(temp), "%d", ptr->end_time);
                strcat(buf, temp);
                send(ptr->connfd, buf, 100, 0);
                close(ptr->connfd);
                if (ptr == singles_queue.front){
                    singles_queue.front = ptr->next;
                    ptr->next->prev = NULL;
                }
                else if (ptr == singles_queue.rear){
                    singles_queue.rear = ptr->prev;
                    ptr->prev->next = NULL;
                }
                else {
                    ptr->prev->next = ptr->next;
                    ptr->next->prev = ptr->prev;
                }
                next = ptr->next;
                free(ptr);
            }
            ptr = next;
        }
        ptr = doubles_queue.front;

        while (ptr) {
            if (globalTime == ptr->end_time){
                char buf[] = "Time out at ";
                char temp[4];
                snprintf(temp, sizeof(temp), "%d", ptr->end_time);
                strcat(buf, temp);
                send(ptr->connfd, buf, 100, 0);
                close(ptr->connfd);
                if (ptr == doubles_queue.front){
                    doubles_queue.front = ptr->next;
                    ptr->next->prev = NULL;
                }
                else if (ptr == doubles_queue.rear){
                    doubles_queue.rear = ptr->prev;
                    ptr->prev->next = NULL;
                }
                else {
                    ptr->prev->next = ptr->next;
                    ptr->next->prev = ptr->prev;
                }
                next = ptr->next;
                free(ptr);
            }
            ptr = next;
        }
        Games* ptr2;
        ptr2 = final_queue.front;
        Games* next2;
        while (ptr2) {
            if (globalTime == ptr2->end_time){
                for (int i = 0; i < ptr2->num_players; i++){
                    char buf[] = "Game Ended at ";
                    char temp[4];
                    snprintf(temp, sizeof(temp), "%d", ptr->end_time);
                    strcat(buf, temp);
                    send(ptr2->players[i], buf, 100, 0);
                    close(ptr2->players[i]);
                    if (ptr2 == final_queue.front){
                        final_queue.front = ptr2->next;
                        ptr2->next->prev = NULL;
                    }
                    else if (ptr2 == final_queue.rear){
                        final_queue.rear = ptr2->prev;
                        ptr2->prev->next = NULL;
                    }
                    else {
                        ptr2->prev->next = ptr2->next;
                        ptr2->next->prev = ptr2->prev;
                    }
                    next2 = ptr2->next;
                    free(ptr2);
                }
            }
            ptr2 = next2;
        }
    }
}

int main() {
    int listenfd, connfd, n;
    socklen_t clilen;
    char buf[MAXLINE];
    struct sockaddr_in cliaddr, servaddr;

    //Create a socket for the soclet
    //If sockfd<0 there was an error in the creation of the socket
    if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
    perror("Problem in creating the socket");
    exit(2);
    }


    //preparation of the socket address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    //bind the socket
    bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    //listen to the socket by creating a connection queue, then wait for clients
    listen (listenfd, LISTENQ);

    printf("%s\n","Server running...waiting for connections.");


    singles_queue.front = singles_queue.rear = NULL;
    singles_queue.count = 0;
    doubles_queue.front = doubles_queue.rear = NULL;
    doubles_queue.count = 0;
    final_queue.front = final_queue.rear = NULL;
    final_queue.count = 0;

    for (int i = 0; i < MAX_COURTS; i++) {
        courts[i].court_number = i + 1;
        courts[i].game_start_time = 0;
        courts[i].endtime = 0;

    }

    #pragma omp parallel num_threads(4)
    {
        #pragma omp sections
        {
            #pragma omp section
            {
                while(true){
                    clilen = sizeof(cliaddr);
                    connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
                        handle_request(connfd);
                }
            }

            #pragma omp section
            {
                singles_function();
            }
            #pragma omp section
            {
                doubles_function();
            }
            #pragma omp section
            {
                final_function();
            }
        }
    }
    return 0;
}

