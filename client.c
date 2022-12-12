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


#define PORT "25893"
#define MAXBUFLEN 100


// get_in_addr function was taken from Beej's Guide to Network Programming
// (6.1 A Simple Stream Server)
void* get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// tcp_connect function heavily inspired by Beej's Guide to Network Programming
// 6.1 A Simple Stream Server
int tcp_connect(char dyn_port[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    struct sockaddr_in sin;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // assign scoket and return descriptor
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    
    // dyn_port will hold the dynamically assigned client-side port number as a
    // string for future print statements
    socklen_t len = sizeof(sin);
    if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else
        sprintf(dyn_port, "%d", ntohs(sin.sin_port));

    freeaddrinfo(servinfo);
    return sockfd;
}

// recv_str receives string messages from serverM and stores it in buf
void recv_str(int sockfd, char buf[])
{
    int numbytes;
    if ((numbytes = recv(sockfd, buf, MAXBUFLEN-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
}

// send_str sends string messages, stored in str, to serverM
int send_str(int sockfd, char str[])
{
    if (send(sockfd, str, strlen(str), 0) == -1)
        perror("send");
}

// The main client loop first prompts user for login information,
// allowing up to 3 attempts to login. If unsuccessful, the client exits.
// If login was successful, it continuously asks for course queries
// up until the client is manually terminated by the user.
int main(int argc, char *argv[])
{
    int numbytes;
    char username[MAXBUFLEN];
    char password[MAXBUFLEN];
    char username_password[MAXBUFLEN]; // store concatenated username and password
    char category[MAXBUFLEN];
    char course[MAXBUFLEN];
    char course_category[MAXBUFLEN]; // store concatenated course code and query category
    char buf_response[MAXBUFLEN]; // stores any response from serverM
    char dyn_port[INET6_ADDRSTRLEN]; // stores client-side dynamically assigned TCP port number
    int sockfd = tcp_connect(dyn_port); // TCP socket descriptor

    printf("The client is up and running.\n");

    int remaining_attempts = 3;

    while (remaining_attempts > 0) {
        remaining_attempts--;
        printf("Please enter the username: ");
        scanf("%s", username);
        username[strcspn(username, "\t\r\n\v\f")] = 0;
        strcpy(username_password, username);
        printf("Please enter the password: ");
        scanf("%s", password);
        password[strcspn(password, "\t\r\n\v\f")] = 0;
        strcat(username_password, ",");
        strcat(username_password, password);

        // send login request to serverM as concatenated username-password string
        send_str(sockfd, username_password);
        printf("%s sent an authentication request to the main server.\n", username);
        // receive login response into buf_response
        recv_str(sockfd, buf_response);
    
        // a response of 2 to the login request represents a SUCCESSFUL login.
        // Subsequently enters a loop of requesting for course-category queries.
        if (strcmp(buf_response, "2") == 0) {
            printf("%s received the result of authentication using TCP over port %s. Authentication is successful\n", username, dyn_port);
            while (1) {
                printf("Please enter the course code to query:");
                scanf("%s", course);
                course[strcspn(course, "\t\r\n\v\f")] = 0;
                strcpy(course_category, course);
                printf("Please enter the category (Credit / Professor / Days / CourseName):");
                scanf("%s", category);
                category[strcspn(category, "\t\r\n\v\f")] = 0;
                strcat(course_category, ",");
                strcat(course_category, category);

                // send course query request to serverM as concatenated course-category string
                send_str(sockfd, course_category);
                printf("%s sent a request to the main server.\n", username);
                recv_str(sockfd, buf_response);
                printf("The client received the response from the Main server using TCP over port %s.\n", dyn_port);
                if (strcmp(buf_response, "None") == 0) {
                    printf("Didn't find the course: %s.\n", course);
                }
                else if (strcmp(buf_response, "NoneCategory") == 0) {
                    printf("Didn't find the category: %s.\n", category);
                }
                else {
                    printf("The %s of %s is %s.\n", category, course, buf_response);
                }
                printf("\n-----Start a new request-----\n");
            }
        }
        // Login attempt response of 1 represents INCORRECT PASSWORD case
        else if (strcmp(buf_response, "1") == 0) {
            printf("%s received the result of authentication using TCP over port %s. Authentication failed: Password does not match\n", username, dyn_port);
        }
        // Login attempt response of anything else represents INCORRECT USERNAME case
        else {
            printf("%s received the result of authentication using TCP over port %s. Authentication failed: Username Does not exist\n", username, dyn_port);
        }
        printf("\nAttempts remaining:%d\n", remaining_attempts);
    }
    printf("Authentication Failed for 3 attempts. Client will shut down.\n");
    close(sockfd);
    return 0;
}
