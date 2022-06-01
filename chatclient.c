/*
    August Lautt

    Description:
    Client-side TCP chat application using socket api to connect to a server
    and transmit message back and forth. Additional information available in
    the readme.
    Usage:
    chatclient [hostname] [portnumber]
*/

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

#define MAXDATASIZE 513 // includes room for handle, "> ", and \0
#define MAXMSGSIZE 500
#define HANDLESIZE 12 //room for \n and \0


void *get_in_addr(struct sockaddr *sa);
int makeConn(char hostName[], char portNum[]);
int receiveMsg(int sockfd);
int sendMsg(int sockfd, char *handle, int firstMsg, char portNum[]);

int main(int argc, char *argv[])
{
    int sockfd, chatting = 1;
    char handle[HANDLESIZE];

    memset(handle, '\0', sizeof handle);

    // check for 3 command line arguments
    if (argc != 3)
    {
        fprintf(stderr, "Usage: chatclient [hostname] [portnumber]\n");
        exit(1);
    }

    printf("Welcome to the chat client!\n");

    // make connection using hostname and port number
    sockfd = makeConn(argv[1], argv[2]);

    // get client handle and strip the newline
    printf("Enter handle: ");
    fgets(handle, HANDLESIZE - 1, stdin);
    strtok(handle, "\n");
    //fseek(stdin, 0, SEEK_END);

    // send initial message with port number
    sendMsg(sockfd, handle, 1, argv[2]);

    // receive and send messages until quit entered or received
    while (chatting)
    {
        chatting = receiveMsg(sockfd);
        if (chatting != 0)
        {
            chatting = sendMsg(sockfd, handle, 0, argv[2]);
        }
    }

    // close the connection and exit program
    close(sockfd);
    printf("Connection to server terminated.\n");

    return 0;
}


/*
    get_in_addr(): Return either pointer to the IPv6 or IPv4 address.
    Takes hostname and portNum, which are the arguments passed in the command line.
    Returns socket file descriptor for further TCP connection functionality.
*/
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


/*
    makeConn(): Make a connection to server by filling required server
    information with getaddrinfo(), creating the socket with socket(), and finally
    connect to a server address using our socket.

    Takes hostname and portNum, which are the arguments passed in the command line.
    Returns socket file descriptor for further TCP connection functionality.
*/
int makeConn(char hostName[], char portNum[])
{
    int sockfd, status;
    struct addrinfo hints, *servinfo, *p;
    char ipstr[INET6_ADDRSTRLEN];

    // empty the address info to fill with data
    memset(&hints, 0, sizeof hints);

    // set to AF_UNSPEC for either IPv4 or IPv6 and set to socket stream for TCP
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // set servinfo to point to addrinfo struct with additional hostname, port info
    if ((status = getaddrinfo(hostName, portNum, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    // set pointer to serverinfo and iterate through linked list until server found
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        // create the socket using servinfo data we filled
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        // this is a client so we connect instead of bind using the created socket
        // and the addr in the servinfo
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        exit(1);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), ipstr, sizeof ipstr);
    printf("Connecting to %s\n", ipstr);

    // free the servinfo struct since it is no longer necessary
    freeaddrinfo(servinfo);

    // return the socket file descriptor so we can use it in other socket functionality
    return sockfd;
}


/*
    receiveMsg(): Receives incoming data and either print incoming message or
    prints that server has disconnected.

    Takes socket file descriptor to pass to recv.
    Returns 1 if message was received and to continue chat or 0 if not.
*/
int receiveMsg(int sockfd)
{
    char buffer[MAXDATASIZE];

    memset(buffer, '\0', sizeof buffer);

    // receive data into buffer
    int numbytes = recv(sockfd, buffer, MAXDATASIZE-1, 0);

    if (numbytes == -1)
    {
        perror("recv");
        exit(1);
    }
    // if recv returns 0, that means server disconnected
    else if (numbytes == 0)
    {
        // so print info and return 0 to prepare for exit
        printf("Server disconnected from client.\n");
        return 0;
    }

    // otherwise, print the buffer and return 1 to continue
    printf("%s\n", buffer);

    return 1;
}


/*
    sendMsg(): Prompts user for message text, then either sends data or sends
    quit data and flags the program to quit.

    Takes socket file descriptor to pass to send, the client's handle to prepend,
    a bool for if this is the first message or not, and the port number.
    Returns 1 if message was sent and to continue chat or 0 if not.
*/
int sendMsg(int sockfd, char *handle, int firstMsg, char portNum[])
{
    int numbytes;
    char buffer[MAXDATASIZE];
    char outgoing[MAXDATASIZE];

    memset(buffer, '\0', sizeof buffer);
    memset(outgoing, '\0', sizeof outgoing);

    // add handle to outgoing message
    strcat(outgoing, handle);

    // if this is the first message, add the port number to automatically send
    if (firstMsg)
    {
        strcat(outgoing, "> ");
        strcat(outgoing, portNum);
    }
    // otherwise, prompt user for text and format it for transfer
    else
    {
        printf("%s> ", handle);
        fgets(buffer, MAXMSGSIZE, stdin);
        strtok(buffer, "\n");

        // if the user entered quit command send that to the server
        if (strncmp(buffer, "\\quit", 5) == 0)
        {
            if ((numbytes = send(sockfd, buffer, MAXDATASIZE, 0)) == -1)
            {
                perror("send");
                exit(1);
            }
            // return 0 to indicate client has chosen to quit
            return 0;
        }

        strcat(outgoing, "> ");
        strcat(outgoing, buffer);
    }

    // otherwise, send the message using socket file descriptor
    if ((numbytes = send(sockfd, outgoing, MAXDATASIZE, 0)) == -1)
    {
        perror("send");
        exit(1);
    }

    // return 1 to indicate client wants to continue chat
    return 1;
}
