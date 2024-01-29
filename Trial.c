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
    sem_t lock;
} Court;

Queue singles_queue, doubles_queue;
Court courts[MAX_COURTS];
pthread_t reader_thread, singles_thread, doubles_thread;
bool singles_queue_empty = false;
bool doubles_queue_empty = false;
bool enquing_done = false;

void* reader_function(void* arg);
void* singles_function(void* arg);
void* doubles_function(void* arg);
void enqueue(Queue* queue, int player_id, int arrival_time, char gender, char preference);
void dequeue(Queue* queue, bool* empty);
void schedule_game(Queue* queue, int court_number, int numplayers);

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
        sem_init(&courts[i].lock, 0, 1); // Initialize each court semaphore with value 1 (unlocked)
    }

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
    return 0;
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
            enqueue(&singles_queue, pid, arr_time, sex[0], preference[0]);
        }
        else if (preference[0] == 'D' || preference[0] == 'B') {
            enqueue(&doubles_queue, pid, arr_time, sex[0], preference[0]);
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

void* singles_function(void* arg) {

    while (1) {
        printf("count :%d\n",singles_queue.count);
        if (singles_queue.count < 2/*singles_queue.front == NULL || singles_queue.front->next == NULL*/) {
            if (enquing_done){
                break;
            }
            continue;
        }
        for (int i = 0; i < MAX_COURTS; i++) {
            sem_wait(&courts[i].lock); // Acquire lock on the court
            // int count = 0;
            // PlayerNode* ptr = singles_queue.front;
            // while (count <= 2 && ptr != NULL) {
            //     ptr = ptr->next;
            //     count += 1;
            // }
            // if (count != 2) {
            //     break;
            // }
            if (courts[i].endtime < singles_queue.front->next->arrival_time) {
                schedule_game(&singles_queue, i, 2); // Schedule singles game on court i
                dequeue(&singles_queue, &singles_queue_empty);
                dequeue(&singles_queue, &singles_queue_empty);
                printf("empty? :%d\n",singles_queue_empty);
                sem_post(&courts[i].lock); // Release lock on the court
                break;
            }
        }
        if (enquing_done && singles_queue_empty){
            break;
        }
    }
    printf("Sucessfully exiting single's func\n");
    return NULL;
}

void* doubles_function(void* arg) {
    while (1) {
        if (doubles_queue.count < 4) {
            if (enquing_done){
                break;
            }
            continue;
        }
        printf("This should not have been printed\n");
        for (int i = 0; i < MAX_COURTS; i++) {
            sem_wait(&courts[i].lock); // Acquire lock on the court
            if (courts[i].endtime < doubles_queue.front->next->next->next->arrival_time) {
                schedule_game(&doubles_queue, i, 4); // Schedule doubles game on court i
                dequeue(&doubles_queue, &doubles_queue_empty);
                dequeue(&doubles_queue, &doubles_queue_empty);
                dequeue(&doubles_queue, &doubles_queue_empty);
                dequeue(&doubles_queue, &doubles_queue_empty);
            }
            sem_post(&courts[i].lock); // Release lock on the court
        }
        if (enquing_done && doubles_queue.count == 0){
            break;
        }
    }
    printf("Sucessfully exiting double's func\n");
    return NULL;
}

void enqueue(Queue* queue, int player_id, int arrival_time, char gender, char preference) {
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
        endtime = (numplayers == 2) ? 5 : 10; // If no male players, end time based on game type
    } else {
        endtime = (numplayers == 2) ? 10 : 15; // If male players present, end time based on game type
    }

    // Update court state
    if (numplayers == 2){
        courts[court_number].game_start_time = queue->front->next->arrival_time;
        // printf("start time: %d\n", queue->front->next->arrival_time);
    }
    else {
        courts[court_number].game_start_time = queue->front->next->next->next->arrival_time;
    }
    // courts[court_number].game_start_time = queue->front->arrival_time;
    // printf("start time: %d\n",courts[court_number].game_start_time);
    endtime += courts[court_number].game_start_time;
    courts[court_number].endtime = endtime; 
    // Output to CSV file
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
