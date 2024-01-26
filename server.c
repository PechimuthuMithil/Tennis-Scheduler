#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>


#define MAXLINE 5120 /*max text line length*/
#define SERV_PORT 3000 /*port*/
#define LISTENQ 8 /*maximum number of client connections*/
#define MAX_COURTS 4
#define MAX_QUEUE_SIZE 100
// #define MAX_SLOTS 8

struct court {
  int numplayers;
  int players[4];
  int endtime;
  sem_t access;
};

// Struct to represent each element in the queue
typedef struct {
    char data[MAXLINE];
} QueueElement;

// Struct to represent the queue
typedef struct {
    QueueElement elements[MAX_QUEUE_SIZE];
    int start;
    int end;
    int size;
} SharedQueue;

// Create or attach to shared memory for the queue
SharedQueue* createSharedQueue(int* shmid2) {
    key_t key = ftok("/tmp", 'Q'); // Unique key for the shared memory segment

    // Create or get the shared memory segment
    int shmid = shmget(key, sizeof(SharedQueue), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Attach to the shared memory segment
    SharedQueue* sharedQueue = (SharedQueue*)shmat(shmid, NULL, 0);
    if (sharedQueue == (SharedQueue*)-1) {
        perror("shmat");
        exit(1);
    }

    // Initialize the queue
    sharedQueue->start = 0;
    sharedQueue->end = -1;
    sharedQueue->size = 0;

    return sharedQueue;
}

// Enqueue an element into the shared queue
void enqueue(SharedQueue* queue, const char* element) {
    if (queue->size == MAX_QUEUE_SIZE) {
        fprintf(stderr, "Queue is full. Cannot enqueue.\n");
        return; // Queue is full, cannot enqueue
    }

    queue->end = (queue->end + 1) % MAX_QUEUE_SIZE;
    strcpy(queue->elements[queue->end].data, element);
    queue->size++;
}

// Dequeue an element from the shared queue
void dequeue(SharedQueue* queue) {
    if (queue->size == 0) {
        fprintf(stderr, "Queue is empty. Cannot dequeue.\n");
        return; // Queue is empty, cannot dequeue
    }

    queue->start = (queue->start + 1) % MAX_QUEUE_SIZE;
    queue->size--;
}

// Search for an element in the shared queue based on Player-ID
int searchQueue(SharedQueue* queue, const char* playerId) {
    for (int i = 0; i < queue->size; i++) {
        int index = (queue->start + i) % MAX_QUEUE_SIZE;
        if (strstr(queue->elements[index].data, playerId) != NULL) {
            return index; // Return the index if found
        }
    }

    return -1; // Return -1 if not found
}

// Display the elements in the shared queue
void displayQueue(SharedQueue* queue) {
    printf("Queue elements:\n");
    for (int i = 0; i < queue->size; i++) {
        int index = (queue->start + i) % MAX_QUEUE_SIZE;
        printf("%s\n", queue->elements[index].data);
    }
}

// Detach from shared memory
void detachSharedQueue(SharedQueue* queue) {
    shmdt(queue);
}

// Remove the shared memory segment
void removeSharedQueue(int shmid) {
    shmctl(shmid, IPC_RMID, NULL);
}

int main (int argc, char **argv)
{
    int shmid;
    shmid = shmget(IPC_PRIVATE, MAX_COURTS * sizeof(struct court), IPC_CREAT | 0666);
    if (shmid == -1) {
    perror("shmget");
    return 1;
    }

    /* Attach the shared memory segment. */
    struct court (courts)[MAX_COURTS] = shmat(shmid, NULL, 0);
    if (courts == (struct court(*)[MAX_COURTS])-1) {
    perror("shmat");
    return 1;
    }

    // Initialize the time_slots

    for (int i = 0; i < MAX_COURTS; i++) {

    courts[i].numplayers = 0;
    for (int j = 0; j < 4; j++) {
        courts[i].players[j] = 0;
    }
    courts[i].endtime = -1;
    sem_init(&courts[i].access, 1, 1);
    }
    int* shmid2;
    SharedQueue* sharedQueue = createSharedQueue(shmid2);

    int listenfd, connfd, n;
    pid_t childpid;
    socklen_t clilen;
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

    for ( ; ; ) {

    clilen = sizeof(cliaddr);
    //accept a connection
    connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);

    printf("%s\n","Received request...");

    if ( (childpid = fork ()) == 0 ) {//if it’s 0, it’s child process
        char buf[MAXLINE];
        printf ("%s\n","Child created for dealing with client requests");

        //close listening socket
        close (listenfd);

        n = recv(connfd, buf, MAXLINE,0);
        puts(buf);

        if (n < 0) {
        printf("%s\n", "Read error");
        }

        else {

        }
        close(connfd);
        printf("Finished serving a child\n");
        exit(0);
    }
    //close socket of the server
    close(connfd);
    }
    shmdt(courts);
    shmctl(shmid, IPC_RMID, NULL);
    detachSharedQueue(sharedQueue);
    removeSharedQueue(*shmid2);
    return 0;

}