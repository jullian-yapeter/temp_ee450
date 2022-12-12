#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>


#define PORT "22893"
#define MAXBUFLEN 200


char** cs_txt_content;  // store CS courses data
int len_cs_txt_content;  // number of lines of CS courses file


// get_in_addr function was taken from Beej's Guide to Network Programming
// (6.3 Datagram Sockets)
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// start_udp_server function was heavily inspired by Beej's Guide to Network Programming
// (6.3 Datagram Sockets, listener.c)
int start_udp_server()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // assign socket and return descriptor
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }
    freeaddrinfo(servinfo);
    return sockfd;
}

// read_and_store_cs_txt reads cs.txt which it assumes
// is located in the same directory as the serverCS executable file,
// and stores the content in memory.
void read_and_store_cs_txt() {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    // get number of lines in the file
    int num_lines = 0;
    fp = fopen("cs.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    while ((read = getline(&line, &len, fp)) != -1)
        num_lines++;
    fclose(fp);

    len_cs_txt_content = num_lines;
    
    // allocate memory to the CS course data storage data structure
    cs_txt_content = malloc(num_lines * sizeof(char*));
    for (int i = 0; i < num_lines; i++)
        cs_txt_content[i] = malloc((MAXBUFLEN + 1) * sizeof(char));

    // store cs.txt data in local data structure
    fp = fopen("cs.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    int line_idx = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
        strcpy(cs_txt_content[line_idx], line);
        line_idx++;
        // if (line)
        //     free(line);
    }
    fclose(fp);
}

// check_cs_data loops through the locally stored CS courses data, and compares the
// specified course data request to the stored data; returning a success/failure
// code to the client
char* check_cs_data(char course_category[])
{
    char* token;
    int code;

    token = strtok(course_category, ",");
    char course[MAXBUFLEN];
    strcpy(course, token);
    token = strtok(NULL, ",");
    char category[MAXBUFLEN];
    strcpy(category, token);

    printf("The ServerCS received a request from the Main Server about the %s of %s.\n", category, course);

    // loop through stored CS courses data to compare with request
    char line[MAXBUFLEN];
    for (int i = 0; i < len_cs_txt_content; i++) {
        strcpy(line, cs_txt_content[i]);
        token = strtok(line, ",");
        token[strcspn(token, "\t\r\n\v\f")] = 0;
        if (strcmp(course, token) == 0) {
            int category_idx = 0;
            if (strcmp(category, "Credit") == 0) {
                category_idx = 1;
            }
            else if (strcmp(category, "Professor") == 0) {
                category_idx = 2;
            }
            else if (strcmp(category, "Days") == 0) {
                category_idx = 3;
            }
            else if (strcmp(category, "CourseName") == 0) {
                category_idx = 4;
            }
            if (category_idx > 0) {
                for (int i = 0; i < category_idx; i++) {
                    token = strtok(NULL, ",");
                }
                token[strcspn(token, "\t\r\n\v\f")] = 0;
                printf("The course information has been found: The %s of %s is %s.\n", category, course, token);
                return token;
            }
        }
    }
    printf("Didn't find the course: %s.\n", course);
    return "None"; // wrong course code
}

// udp_recv_and_respond receives a request from the client over UDP and
// responds to the client accordingly
void udp_recv_and_respond(int sockfd, struct sockaddr_storage their_addr, socklen_t addr_len, char buf[])
{
    addr_len = sizeof their_addr;
    int numbytes;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                             (struct sockaddr *)&their_addr, &addr_len)) == -1){
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';

    // check received course data request
    char* resp = check_cs_data(buf);

    // send response to serverM (requested data/ failure code)
    if ((numbytes = sendto(sockfd, resp, strlen(resp), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
        perror("senderr: sendto");
        exit(1);
    }
    printf("The ServerCS finished sending the response to the Main Server.\n");
}

int main(void)
{
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    // start UDP listener
    int sockfd = start_udp_server();
    // read and store cs.txt data
    read_and_store_cs_txt();

    /*
    // Code to check local CS data stored:
    printf("num lines: %d\n", len_cred_txt_content);
    for (int i = 0; i < len_cs_txt_content; i++) {
        printf("%s",cred_cs_content[i]);
    }
    */

    // loop to service CS data requests
    printf("The ServerCS is up and running using UDP on port %s.\n", PORT);
    while(1) {
        udp_recv_and_respond(sockfd, their_addr, addr_len, buf);
    }

    close(sockfd);
    return 0;
}
