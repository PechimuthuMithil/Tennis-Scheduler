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
typedef struct Node {
    char data[MAXLINE];
    struct Node* prev;
    struct Node* next;
} Node;

// Struct to represent the doubly linked list
typedef struct {
    Node* start;
    Node* end;
    int size;
} SharedLinkedList;

// Create or attach to shared memory for the linked list
SharedLinkedList* createSharedLinkedList(int* shmid) {
    key_t key = ftok("/tmp", 'L'); // Unique key for the shared memory segment

    // Create or get the shared memory segment
    *shmid = shmget(key, sizeof(SharedLinkedList), 0666 | IPC_CREAT);
    if (*shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Attach to the shared memory segment
    SharedLinkedList* sharedLinkedList = (SharedLinkedList*)shmat(*shmid, NULL, 0);
    if (sharedLinkedList == (SharedLinkedList*)-1) {
        perror("shmat");
        exit(1);
    }

    // Initialize the linked list
    sharedLinkedList->start = NULL;
    sharedLinkedList->end = NULL;
    sharedLinkedList->size = 0;
    return sharedLinkedList;
}

// Insert an element at the end of the shared linked list
void insert(SharedLinkedList* list, const char* element) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    strcpy(newNode->data, element);
    newNode->prev = list->end;
    newNode->next = NULL;

    if (list->end != NULL) {
        list->end->next = newNode;
    } else {
        list->start = newNode;
    }
    list->end = newNode;

    list->size++;
}

// Remove the first element from the shared linked list
void removeFirst(SharedLinkedList* list) {
    if (list->size == 0) {
        fprintf(stderr, "List is empty. Cannot remove.\n");
        return;
    }

    Node* temp = list->start;
    list->start = temp->next;
    if (list->start != NULL) {
        list->start->prev = NULL;
    } else {
        list->end = NULL;
    }

    free(temp);
    list->size--;
}

// Search for an element in the shared linked list based on Player-ID
int searchList(SharedLinkedList* list, char* req_pref) {
    Node* current = list->start;
    int index = 0;
    while (current != NULL) {
        char* req = current->data;
        int pid, arr_time;
        char* sex;
        char* preference;
        pid = atoi(strtok(req, ","));
        arr_time = atoi(strtok(NULL, ","));
        sex = strtok(NULL, ",");
        size_t len_sex = strlen(sex);
        if (len_sex > 0 && sex[len_sex-1] == '\n'){
            sex[len_sex-1] = '\0';
        }
        preference = strtok(NULL, ",");
        size_t len_pref = strlen(preference);
        if (len_pref > 0 && preference[len_pref-1] == '\n'){
            preference[len_pref-1] = '\0';
        }
        if (strcmp(preference, req_pref) == 0) {
            return index; // Return the index if found
        }
        current = current->next;
        index++;
    }

    return -1; // Return -1 if not found
}

// Display the elements in the shared linked list
void displayList(SharedLinkedList* list) {
    printf("List elements:\n");
    Node* current = list->start;
    while (current != NULL) {
        printf("%s\n", current->data);
        current = current->next;
    }
}

// Detach from shared memory
void detachSharedLinkedList(SharedLinkedList* list) {
    shmdt(list);
}

// Remove the shared memory segment
void removeSharedLinkedList(int shmid) {
    shmctl(shmid, IPC_RMID, NULL);
}


int handle_req(struct court* courts, char* req, SharedLinkedList* list){
    int pid, arr_time;
    char* sex;
    char* preference;
    pid = atoi(strtok(req, ","));
    arr_time = atoi(strtok(NULL, ","));
    sex = strtok(NULL, ",");
    size_t len_sex = strlen(sex);
    if (len_sex > 0 && sex[len_sex-1] == '\n'){
        sex[len_sex-1] = '\0';
    }
    preference = strtok(NULL, ",");
    size_t len_pref = strlen(preference);
    if (len_pref > 0 && preference[len_pref-1] == '\n'){
        preference[len_pref-1] = '\0';
    }
    if (list->size == 0){
        insert(list,req);
        return 0;
    }
    // Analyzing what request needs
    // Find an appropriate opponent
    if (strcmp(preference,"S") == 0) {
         
    }
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


    for (int i = 0; i < MAX_COURTS; i++) {

    courts[i].numplayers = 0;
    for (int j = 0; j < 4; j++) {
        courts[i].players[j] = 0;
    }
    courts[i].endtime = -1;
    sem_init(&courts[i].access, 1, 1);
    }
    int* shmid2;
    SharedLinkedList* sharedLinkedList = createSharedLinkedList(shmid2);

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
            // Write code here.
            int res = handle_req(courts,buf,sharedLinkedList);
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
    detachSharedLinkedList(sharedLinkedList);
    removeSharedLinkedList(*shmid2);
    return 0;

}