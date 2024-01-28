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
#define MAX_QUEUE_SIZE 100
#define MAXLINE 5120 /* max text line length */
#define MAX_ROW_LENGTH 5120

typedef struct {
    int player_id;
    int arrival_time;
    char gender;
    char preference;
} Player;

typedef struct {
    Player players[MAX_QUEUE_SIZE];
    int front;
    int rear;
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

void* reader_function(void* arg);
void* singles_function(void* arg);
void* doubles_function(void* arg);
void enqueue(Queue* queue, Player player);
Player dequeue(Queue* queue);
void schedule_game(Queue* queue, int court_number, int numplayers);

int main(int argc, char **argv) {
    // Initialize queues
    FILE* reqs;
    FILE* output;
    char req[MAX_ROW_LENGTH];

    reqs = fopen(argv[1], "r");
    if (reqs == NULL) {
        perror("Error opening request file");
        exit(5);
    }
    fgets(req, MAX_ROW_LENGTH, reqs);

    // Initialize courts
    for (int i = 0; i < MAX_COURTS; i++) {
        courts[i].court_number = i + 1;
        courts[i].endtime = 0;
        sem_init(&courts[i].lock, 0, 1); // Initialize each court semaphore with value 1 (unlocked)
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

    return 0;
}

void* reader_function(void* arg) {
    FILE* reqs = (FILE*)arg;
    char req[MAX_ROW_LENGTH];
    while (fgets(req, MAX_ROW_LENGTH, reqs) != NULL) {
        printf("%s\n",req);
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

        Player player;
        player.player_id = pid;
        player.arrival_time = arr_time;
        player.gender = sex[0]; // Assuming gender is a single character
        player.preference = preference[0]; // Assuming preference is a single character
        
        // Enqueue the player based on preference
        if (player.preference == 'S' || player.preference == 'b') {
            printf("Enquing.%d\n",player.player_id);
            enqueue(&singles_queue, player);
        }
        else if (player.preference == 'D' || player.preference == 'B') {
            enqueue(&doubles_queue, player);
        }
        else {
            fprintf(stderr, "Invalid preference for player %d\n", pid);
        }
    }
    return NULL;
}


void* singles_function(void* arg) {
    while (1) {
        if (singles_queue.count < 2) {
            continue;
        }
        printf("Started\n");
        for (int i = 0; i < MAX_COURTS; i++) {
            sem_wait(&courts[i].lock); // Acquire lock on the court
            if (courts[i].endtime < singles_queue.players[singles_queue.front].arrival_time) {
                schedule_game(&singles_queue, i, 2); // Schedule singles game on court i
            }
            sem_post(&courts[i].lock); // Release lock on the court
        }
    }
    return NULL;
}

void* doubles_function(void* arg) {
    while (1) {
        if (doubles_queue.count < 2) {
            continue;
        }

        for (int i = 0; i < MAX_COURTS; i++) {
            sem_wait(&courts[i].lock); // Acquire lock on the court
            if (courts[i].endtime < doubles_queue.players[doubles_queue.front].arrival_time) {
                schedule_game(&doubles_queue, i, 4); // Schedule singles game on court i
            }
            sem_post(&courts[i].lock); // Release lock on the court
        }
    }
    return NULL;
}

void enqueue(Queue* queue, Player player) {
    if (queue->count < MAX_QUEUE_SIZE) {
        queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
        queue->players[queue->rear] = player;
        queue->count++;
    }
}

Player dequeue(Queue* queue) {
    Player player;
    if (queue->count > 0) {
        player = queue->players[queue->front];
        queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
        queue->count--;
    }
    return player;
}

void schedule_game(Queue* queue, int court_number, int numplayers) {
    printf("Game scheduled on court %d\n", court_number + 1);
    int endtime;

    // Calculate end time based on the number of male players and game type
    int num_male_players = 0;
    for (int i = 0; i < numplayers; i++) {
        if (queue->players[queue->front].gender == 'M') {
            num_male_players++;
        }
        queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
        queue->count--;
    }

    if (num_male_players == 0) {
        endtime = (numplayers == 2) ? 5 : 10; // If no male players, end time based on game type
    } else {
        endtime = (numplayers == 2) ? 10 : 15; // If male players present, end time based on game type
    }

    // Update court state
    courts[court_number].game_start_time = queue->players[queue->front].arrival_time;
    endtime += courts[court_number].game_start_time;
    courts[court_number].endtime = endtime;

    // Output to CSV file
    printf("Game-start-time: %d, Court-Number: %d, List-of-player-ids: ", 
           courts[court_number].game_start_time, courts[court_number].court_number);
    for (int i = 0; i < numplayers; i++) {
        printf("%d ", queue->players[(queue->front + i) % MAX_QUEUE_SIZE].player_id);
    }
    printf("\n");
}
