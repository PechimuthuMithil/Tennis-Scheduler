#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdbool.h>

#define MAXLINE 5120 /* max text line length */
#define SERV_PORT 3000 /* port */
#define MAX_ROW_LENGTH 5120

int main(int argc, char **argv) {
    FILE* reqs;
    FILE* output;
    char req[MAX_ROW_LENGTH];
    char sendline[MAXLINE], recvline[MAXLINE];

    reqs = fopen(argv[1], "r");
    if (reqs == NULL) {
        perror("Error opening request file");
        exit(5);
    }
    // Output can be written by the server itself right?
    // However some synchronization will be required.

    // output = fopen(argv[2], "w+");
    // if (reqs == NULL) {
    //     perror("Error opening output file file");
    //     exit(5);
    // }

    int sockfd;
    struct sockaddr_in servaddr;

    char Server_IP[10] = "127.0.0.1";
    fgets(req, MAX_ROW_LENGTH, reqs); //remove headers.
    // size_t len = strlen(req);
    // if (len > 0 && req[len - 1] == '\n') {
    //     req[len - 1] = '\0';
    // }
    // strcat(req, ",Status\n");

    // // Write the modified line to the output file
    // fprintf(output, "%s", req);
    while (feof(reqs) != true) {
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Problem in creating the socket");
            exit(6);
        }

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(Server_IP);
        servaddr.sin_port = htons(SERV_PORT);
        if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            perror("Problem in connecting to the server");
            exit(3);
        }
        fgets(req, MAX_ROW_LENGTH, reqs);
        // size_t len_req = strlen(req);

        printf("Request: %s", req);
        strcpy(sendline, req);
        send(sockfd, sendline, strlen(sendline), 0);
        // sleep(1);
        recv(sockfd, recvline, MAXLINE, 0);
        // size_t len2 = strlen(recvline);

        printf("\nString received from the server: %s\n", recvline);
        printf("-----------------------------------\n");

        // if (len2 > 2) {
        //     size_t len = strlen(req);
        //     if (len > 0 && req[len - 1] == '\n') {
        //         req[len - 1] = '\0';
        //     }
        //     strcat(req, ",");
        //     strcat(req, "0");
        //     strcat(req, ",");
        //     strcat(req, recvline);
        //     strcat(req, "\n");
        //     fprintf(output, "%s", req);
        // } 
        // else {
        //     size_t len = strlen(req);
        //     if (len > 0 && req[len - 1] == '\n') {
        //         req[len - 1] = '\0';
        //     }
        //     strcat(req, ",");
        //     strcat(req, recvline);
        //     strcat(req,",");
        //     strcat(req, "\n");

        //     fprintf(output, "%s", req);
        // }
        close(sockfd);
    }
    fclose(reqs);
    fclose(output);
    exit(0);
    return 0;
}