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
#include <signal.h>
#include <stdbool.h>

#define PORT "25893"
#define UDP_PORT "24893"
#define SERVERCPORT "21893"
#define SERVERCSPORT "22893"
#define SERVEREEPORT "23893"

#define MAXBUFLEN 100
#define BACKLOG 10

// sigchld_handler function was taken from Beej's Guide to Network Programming
// (6.1 A Simple Stream Server)
void sigchld_handler(int s)
{
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// get_in_addr function was taken from Beej's Guide to Network Programming
// (6.1 A Simple Stream Server)
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// start_tcp_server function was heavily inspired by Beej's Guide to Network Programming
// (6.1 A Simple Stream Server, server.c)
int start_tcp_server()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sigaction sa;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // return descriptor
    return sockfd;
}

// start_udp_client function was heavily inspired by Beej's Guide to Network Programming
// (6.3 Datagram Sockets, talker.c)
int start_udp_client()
{
    int udp_sockfd;
	struct addrinfo udp_hints, *udp_servinfo, *udp_p;
	int udp_rv;
	char udp_s[INET6_ADDRSTRLEN];

	memset(&udp_hints, 0, sizeof udp_hints);
	udp_hints.ai_family = AF_UNSPEC;
	udp_hints.ai_socktype = SOCK_DGRAM;
	udp_hints.ai_flags = AI_PASSIVE;

	if ((udp_rv = getaddrinfo(NULL, UDP_PORT, &udp_hints, &udp_servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(udp_rv));
		return 1;
	}

	for(udp_p = udp_servinfo; udp_p != NULL; udp_p = udp_p->ai_next) {
		if ((udp_sockfd = socket(udp_p->ai_family, udp_p->ai_socktype,
				udp_p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
		if (bind(udp_sockfd, udp_p->ai_addr, udp_p->ai_addrlen) == -1) {
			close(udp_sockfd);
			perror("listener: bind");
			continue;
		}
		break;
	}

	if (udp_p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(udp_servinfo);

    // return descriptor
    return udp_sockfd;
}

// configure_udp_server function was heavily inspired by Beej's Guide to Network Programming
// (6.3 Datagram Sockets)
struct addrinfo* configure_udp_server(char server_port[])
{
    struct addrinfo udp_hints, *udp_servinfo, *udp_p;
	int udp_rv;

	memset(&udp_hints, 0, sizeof udp_hints);
	udp_hints.ai_family = AF_UNSPEC;
	udp_hints.ai_socktype = SOCK_DGRAM;

	if ((udp_rv = getaddrinfo("127.0.0.1", server_port, &udp_hints, &udp_servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(udp_rv));
		return NULL;
	}

	udp_p = udp_servinfo;
    return udp_p;
}

// recv_str receives string messages through TCP from client and stores it in buf
void recv_str(int sockfd, char buf[])
{
    int numbytes;
    if ((numbytes = recv(sockfd, buf, MAXBUFLEN-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
}

// send_str sends string, str, messages through TCP to client
int send_str(int sockfd, char str[])
{
    if (send(sockfd, str, strlen(str), 0) == -1)
        perror("send");
}

// udp_receive receives string messages from servers C/CS/EE and stores it in buf
void udp_receive(int sockfd, struct sockaddr_storage their_addr, socklen_t addr_len, char buf[])
{
    int numbytes;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1){
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';
}

// udp_send sends string messages, str, to servers C/CS/EE
void udp_send(int udp_sockfd, struct addrinfo* udp_p, char str[])
{
    int numbytes;
    if ((numbytes = sendto(udp_sockfd, str, strlen(str), 0, udp_p->ai_addr, udp_p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
}

// encrypt_char encrypts an individual character by shifting the character by 4
char encrypt_char(char ch)
{
    // capital letters
    if (ch >= 65 && ch <= 90) {
        ch += 4;
        if (ch > 90) {
            int diff = ch - 91;
            ch = 65 + diff;
        }
    }
    // lowercase letters
    else if (ch >= 97 && ch <= 122) {
        ch += 4;
        if (ch > 122) {
            int diff = ch - 123;
            ch = 97 + diff;
        }
    }
    // digits
    else if (ch >= 48 && ch <= 57) {
        ch += 4;
        if (ch > 57) {
            int diff = ch - 58;
            ch = 48 + diff;
        }
    }
    return ch;
}

// encrypt encrypts a string by shifting each character by 4
void encrypt(char str[])
{
    for (int i = 0; i < strlen(str); i++) {
        str[i] = encrypt_char(str[i]);
    }
}

int main(void)
{
    int new_fd;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    socklen_t sin_size_server;
    struct sockaddr_storage their_addr_server;
    char s[INET6_ADDRSTRLEN];
    bool client_connected = false;
    bool client_authenticated = false;

    // initialize TCP server and UDP client
    int sockfd = start_tcp_server();
    int udp_fd = start_udp_client();
    struct addrinfo* udp_C_p = configure_udp_server(SERVERCPORT);
    struct addrinfo* udp_EE_p = configure_udp_server(SERVEREEPORT);
    struct addrinfo* udp_CS_p = configure_udp_server(SERVERCSPORT);

    char buf_username_password[MAXBUFLEN];
    char buf_username_password_copy[MAXBUFLEN];
    char buf_course_category[MAXBUFLEN];
    char buf_course_category_copy[MAXBUFLEN];
    char buf_department[3];
    char buf_response[MAXBUFLEN];

    printf("The main server is up and running.\n");
    // loop to service client requests
    while(1) {
        // this block is from Beej's Guide to Network Programming (6.1 A Simple Stream Server)
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        if (!fork()) {
            close(sockfd);
            int remaining_attempts = 3;
            client_connected = false;
            client_authenticated = false;
            char* username = NULL;

            // loop through max of 3 attempts for client to login
            while (remaining_attempts > 0) {
                remaining_attempts--;
                // receive login request
                recv_str(new_fd, buf_username_password_copy);
                strcpy(buf_username_password, buf_username_password_copy);
                username = strtok(buf_username_password_copy, ",");
                printf("The main server received the authentication for %s using TCP over port %s.\n", username, PORT);
                encrypt(buf_username_password);
                // send encrypted login request to serverC
                udp_send(udp_fd, udp_C_p, buf_username_password);
                printf("The main server sent an authentication request to serverC.\n");
                // receive login response from serverC and send it to the client
                udp_receive(udp_fd, their_addr_server, sin_size_server, buf_response);
                printf("The main server received the result of the authentication request from ServerC using UDP over port %s.\n", UDP_PORT);
                send_str(new_fd, buf_response);
                printf("The main server sent the authentication result to the client.\n");
                // response of "2" means the authentication was successful, move on to course query stage
                if (strcmp(buf_response, "2") == 0) {
                    client_authenticated = true;
                    break;
                }
            }

            if (client_authenticated) {
                client_connected = true;
                // loop to service course requests
                while(client_connected) {
                    // receive course queries
                    recv_str(new_fd, buf_course_category_copy);
                    strcpy(buf_course_category, buf_course_category_copy);
                    char* course = strtok(buf_course_category_copy, ",");
                    char* category = strtok(NULL, ",");
                    // detect that the client has disconnected
                    if (!course && !category) {
                        client_connected = false;
                        break;
                    }
                    printf("The main server received from %s to query course %s about %s using TCP over port %s.\n", username, course, category, PORT);
                    memcpy(buf_department, buf_course_category, 2);
                    buf_department[2] = '\0';
                    // determine if the request should be sent to the EE server or the CS server.
                    // Then send the request, and receive the requested data/ failure code
                    if (strcmp(buf_department, "EE") == 0) {
                        udp_send(udp_fd, udp_EE_p, buf_course_category);
                        printf("The main server sent a request to serverEE.\n");
                        udp_receive(udp_fd, their_addr_server, sin_size_server, buf_response);
                        printf("The main server received the response from serverEE using UDP over port %s.\n", UDP_PORT);
                        send_str(new_fd, buf_response);
                        printf("The main server sent the query information to the client.\n");
                    }
                    else if (strcmp(buf_department, "CS") == 0) {
                        udp_send(udp_fd, udp_CS_p, buf_course_category);
                        printf("The main server sent a request to serverCS.\n");
                        udp_receive(udp_fd, their_addr_server, sin_size_server, buf_response);
                        printf("The main server received the response from serverCS using UDP over port %s.\n", UDP_PORT);
                        send_str(new_fd, buf_response);
                        printf("The main server sent the query information to the client.\n");
                    }
                    // if the department is not CS or EE, return failure code
                    else {
                        printf("The main server received request with invalid department.\n");
                        send_str(new_fd, "None");
                        printf("The main server sent the query information to the client.\n");
                    }
                }
            }
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }
    return 0;
}
