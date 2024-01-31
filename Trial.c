#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>

#define MAX_PLAYERS 100
#define MAX_COURTS 4
#define MAX_ROW_LENGTH 5120
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct PlayerNode {
    int player_id;
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
    // sem_t lock;
} Court;

Queue singles_queue, doubles_queue;
Court courts[MAX_COURTS];
pthread_t reader_thread, singles_thread, doubles_thread;
sem_t search;
bool singles_queue_empty = true;
bool doubles_queue_empty = true;
bool enquing_done = false;

void* reader_function(void* arg);
void* singles_function(void* arg);
void* doubles_function(void* arg);
void enqueue(Queue* queue, int player_id, int arrival_time, char gender, char preference, bool* empty);
void dequeue(Queue* queue, bool* empty);
void schedule_game(Queue* queue, int court_number, int numplayers);
void enq_front(Queue* queue, int player_id, int arrival_time, char gender, char preference, bool* empty);

int main(int argc, char **argv) {
    // Initialize queues
    singles_queue.front = singles_queue.rear = NULL;
    singles_queue.count = 0;
    doubles_queue.front = doubles_queue.rear = NULL;
    doubles_queue.count = 0;

    // Initialize courts
    for (int i = 0; i < MAX_COURTS; i++) {
        courts[i].court_number = i + 1;
        courts[i].game_start_time = 0;
        courts[i].endtime = 0;
        // sem_init(&courts[i].lock, 0, 1); // Initialize each court semaphore with value 1 (unlocked)
    }
    sem_init(&search,0,1);
    // Open request file
    FILE* reqs = fopen(argv[1], "r");
    if (reqs == NULL) {
        perror("Error opening request file");
        exit(5);
    }

    // Create threads
    pthread_create(&reader_thread, NULL, reader_function, reqs);
    pthread_create(&singles_thread, NULL, singles_function, NULL);
    pthread_create(&doubles_thread, NULL, doubles_function, NULL);
    printf("Hi from main\n");

    // Join threads
    pthread_join(reader_thread, NULL);
    pthread_join(singles_thread, NULL);
    pthread_join(doubles_thread, NULL);

    printf("NEED TO HANDLE EDGE CASE WHEN BOTH QUEUES ARE NOT TOTALLY EMPTY\n");
    if (singles_queue.count == 1){
        if (doubles_queue.count == 1){
            if (doubles_queue.front->preference != 'D'){
                if (doubles_queue.front->arrival_time < singles_queue.front->arrival_time){
                    enq_front(&singles_queue,doubles_queue.front->player_id,doubles_queue.front->arrival_time,doubles_queue.front->gender,doubles_queue.front->preference,&singles_queue_empty);
                }
                else {
                    enqueue(&singles_queue,doubles_queue.front->player_id,doubles_queue.front->arrival_time,doubles_queue.front->gender,doubles_queue.front->preference,&singles_queue_empty);
                }
                singles_function(NULL);
            }
        }
        if (doubles_queue.count == 2){
            PlayerNode* ptr;
            ptr = doubles_queue.front;
            while(ptr){
                if (ptr->preference == 'B') {
                    if (ptr->arrival_time < singles_queue.front->arrival_time){
                        enq_front(&singles_queue,ptr->player_id,ptr->arrival_time,ptr->gender,ptr->preference,&singles_queue_empty);
                    }
                    else {
                        enqueue(&singles_queue,ptr->player_id,ptr->arrival_time,ptr->gender,ptr->preference,&singles_queue_empty);
                    }
                    break;
                }
                ptr = ptr->next;
            }
            singles_function(NULL);
        }
        else {
            if (singles_queue.front->preference == 'b'){
                if (singles_queue.front->arrival_time < doubles_queue.rear->arrival_time){
                    enq_front(&doubles_queue,singles_queue.front->player_id,singles_queue.front->arrival_time,singles_queue.front->gender,singles_queue.front->preference,&singles_queue_empty);
                }
                else {
                    enqueue(&doubles_queue,singles_queue.front->player_id,singles_queue.front->arrival_time,singles_queue.front->gender,singles_queue.front->preference,&singles_queue_empty);
                }
                doubles_function(NULL);
            }
            else {
                PlayerNode* ptr;
                ptr = doubles_queue.front;
                while(ptr){
                    if (ptr->preference == 'B') {
                        if (ptr->arrival_time < singles_queue.front->arrival_time){
                            enq_front(&singles_queue,ptr->player_id,ptr->arrival_time,ptr->gender,ptr->preference,&singles_queue_empty);
                        }
                        else {
                            enqueue(&singles_queue,ptr->player_id,ptr->arrival_time,ptr->gender,ptr->preference,&singles_queue_empty);
                        }
                    }
                    ptr = ptr->next;
                }
                singles_function(NULL);
            }
        }
    }
    return 0;
}

void enq_front(Queue* queue, int player_id, int arrival_time, char gender, char preference, bool* empty){
    PlayerNode* newNode = (PlayerNode*)malloc(sizeof(PlayerNode));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->player_id = player_id;
    newNode->arrival_time = arrival_time;
    newNode->gender = gender;
    newNode->preference = preference;
    newNode->next = queue->front;
    newNode->prev = NULL;

    if (queue->rear == NULL) {
        queue->front = newNode;
    } else {
        queue->front->prev = newNode;
    }
    queue->count++;
    *empty = false;
}

void* reader_function(void* arg) {
    FILE* reqs = (FILE*)arg;
    char req[MAX_ROW_LENGTH];
    fgets(req, MAX_ROW_LENGTH, reqs);
    while (fgets(req, MAX_ROW_LENGTH, reqs) != NULL) {
        int pid, arr_time;
        char* sex;
        char* preference;
        pid = atoi(strtok(req, ","));
        arr_time = atoi(strtok(NULL, ","));
        sex = strtok(NULL, ",");
        preference = strtok(NULL, ",");
        
        // Validate player data
        if (sex == NULL || preference == NULL) {
            fprintf(stderr, "Invalid player data in CSV\n");
            continue; // Skip this line
        }

        // Remove newline characters if present
        size_t len_sex = strlen(sex);
        if (len_sex > 0 && sex[len_sex - 1] == '\n') {
            sex[len_sex - 1] = '\0';
        }
        size_t len_pref = strlen(preference);
        if (len_pref > 0 && preference[len_pref - 1] == '\n') {
            preference[len_pref - 1] = '\0';
        }

        // Enqueue the player based on preference
        if (preference[0] == 'S' || preference[0] == 'b') {
            printf("Enquing.%d\n", pid);
            enqueue(&singles_queue, pid, arr_time, sex[0], preference[0], &singles_queue_empty);
        }
        else if (preference[0] == 'D' || preference[0] == 'B') {
            enqueue(&doubles_queue, pid, arr_time, sex[0], preference[0], &doubles_queue_empty);
        }
        else {
            fprintf(stderr, "Invalid preference for player %d\n", pid);
        }
    }
    enquing_done = true;
    fclose(reqs);
    printf("Sucessfully exiting reader's func\n");
    return NULL;
}

// void* singles_function(void* arg) {
//     int min_endtime_court = 0;
//     while (1) {
//         if (singles_queue.count < 2/*singles_queue.front == NULL || singles_queue.front->next == NULL*/) {
//             if (enquing_done){
//                 break;
//             }
//             continue;
//         }
//         for (int i = 0; i < MAX_COURTS; i++) {
//             sem_wait(&courts[i].lock); // Acquire lock on the court
//             if (courts[i].endtime < singles_queue.front->next->arrival_time) {
//                 schedule_game(&singles_queue, i, 2); // Schedule singles game on court i
//                 dequeue(&singles_queue, &singles_queue_empty);
//                 dequeue(&singles_queue, &singles_queue_empty);
//                 printf("empty? :%d\n",singles_queue_empty);
//                 sem_post(&courts[i].lock); // Release lock on the court
//                 break;
//             }
//             sem_post(&courts[i].lock);
//         }
//         if (enquing_done && singles_queue_empty){
//             break;
//         }
//     }
//     printf("Sucessfully exiting single's func\n");
//     return NULL;
// }
void* singles_function(void* arg) {
    int min_endtime_court = 0;
    while (1) {
        if (singles_queue.count < 2/*singles_queue.front == NULL || singles_queue.front->next == NULL*/) {
            if (enquing_done){
                break;
            }
            continue;
        }
        sem_wait(&search);
        for (int i = 0; i < MAX_COURTS; i++) {
            if (courts[i].endtime < courts[min_endtime_court].endtime) {
                min_endtime_court = i;
            }
        }
        schedule_game(&singles_queue, min_endtime_court, 2); // Schedule singles game on court i
        dequeue(&singles_queue, &singles_queue_empty);
        dequeue(&singles_queue, &singles_queue_empty);
        sem_post(&search); // Release lock on the court
        if (enquing_done && singles_queue_empty){
            break;
        }
    }
    printf("Sucessfully exiting single's func\n");
    return NULL;
}

void* doubles_function(void* arg) {
    int min_endtime_court = 0;
    while (1) {
        if (doubles_queue.count < 4/*singles_queue.front == NULL || singles_queue.front->next == NULL*/) {
            if (enquing_done){
                break;
            }
            continue;
        }
        sem_wait(&search);
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
        sem_post(&search); // Release lock on the court
        printf("empty? %d\n",doubles_queue.count);
        if (enquing_done && doubles_queue_empty){
            break;
        }
    }
    printf("Sucessfully exiting double's func\n");
    return NULL;
}
// void* doubles_function(void* arg) {
//     while (1) {
//         if (doubles_queue.count < 4) {
//             if (enquing_done){
//                 break;
//             }
//             continue;
//         }
//         printf("Entering someplace: %d\n", doubles_queue.count);
//         for (int i = 0; i < MAX_COURTS; i++) {
//             sem_wait(&courts[i].lock); // Acquire lock on the court
//             if (courts[i].endtime < doubles_queue.front->next->next->next->arrival_time) {
//                 schedule_game(&doubles_queue, i, 4); // Schedule doubles game on court i
//                 dequeue(&doubles_queue, &doubles_queue_empty);
//                 dequeue(&doubles_queue, &doubles_queue_empty);
//                 dequeue(&doubles_queue, &doubles_queue_empty);
//                 dequeue(&doubles_queue, &doubles_queue_empty);
//                 printf("empty? :%d\n",doubles_queue_empty);
//                 sem_post(&courts[i].lock);
//                 break;
//             }
//             sem_post(&courts[i].lock);
//         }
//         if (enquing_done && doubles_queue_empty){
//             printf("Entering someplace\n");
//             break;
//         }
        
//     }
//     printf("Sucessfully exiting double's func\n");
//     return NULL;
// }

void enqueue(Queue* queue, int player_id, int arrival_time, char gender, char preference, bool* empty) {
    PlayerNode* newNode = (PlayerNode*)malloc(sizeof(PlayerNode));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->player_id = player_id;
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
    printf("Sucessfully exiting enqueue\n");
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
     printf("Sucessfully exiting dequeue\n");
}

void schedule_game(Queue* queue, int court_number, int numplayers) {
    printf("Game scheduled on court %d\n", court_number + 1);
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
  
    printf("Game-start-time: %d, End time: %d, Court-Number: %d, List-of-player-ids: ", 
           courts[court_number].game_start_time, courts[court_number].endtime, courts[court_number].court_number);
    current = queue->front;
    for (int i = 0; i < numplayers; i++) {
        printf("%d ", current->player_id);
        current = current->next;
    }
    printf("\n");
    printf("Sucessfully exiting schedule_game\n");
}
