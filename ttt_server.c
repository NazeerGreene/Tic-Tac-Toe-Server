//TTT server -- monitor only: prints out game moves sent by client
//IPv4 only
//does not handle ctrl-D, use ctrl-C to terminate session

#include "helpers.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>


#define LISTENING 1
#define ESTABLISHED 2

#define BUFFERLEN 100

int main() {

    //setting up our modifications
    printf("Configuring local address...\n");
    struct addrinfo hints, *bind_address;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = AI_PASSIVE;     //listen on every network interface

    getaddrinfo(0, "6767", &hints, &bind_address);


    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = 
    socket(bind_address->ai_family,
            bind_address->ai_socktype, 
            bind_address->ai_protocol);

    if (!ISVALIDSOCKET(socket_listen)) 
    {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    //binding socket to a local address
    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, 
                bind_address->ai_addrlen)) 
    {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    //free linked list
    freeaddrinfo(bind_address);

    //listening for incoming connections
    printf("Listening...\n");
    //                        v----- backlog (a queue that the OS will manage)
    if (listen(socket_listen, 10) < 0) 
    {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    //client information
    struct sockaddr_in client_sockaddr; //IPv4
    socklen_t client_size = 0;
    SOCKET client;
    char ipv4[INET_ADDRSTRLEN];
    memset(&ipv4, 0, INET_ADDRSTRLEN); //zero out

    //buffer to hold anything the client sends
    char buffer[BUFFERLEN];
    memset(&buffer, 0, BUFFERLEN);

    int STATE = LISTENING;

    while(1) 
    {
            if (STATE == LISTENING) 
        {
            //wait for incoming connections
            printf("Waiting for connections...\n");

            //accept new connections
            client = accept(socket_listen, (struct sockaddr *)&client_sockaddr, &client_size);

            //check for failed connection
            if (client == -1)
            {
                perror("accept failed.");
                //look for next client
                continue;
            }

            //print out new connection
            //covert network to presentation (easy read)
            inet_ntop(AF_INET, &(client_sockaddr.sin_addr), ipv4, INET_ADDRSTRLEN);
            printf("server: got connection from %s\n", ipv4);

            //change state to established
            STATE = ESTABLISHED;
        }
            else if (STATE == ESTABLISHED) 
        {
            int bytes_recv;

            //let's see if there's anything new
            bytes_recv = recv(client, buffer, BUFFERLEN, 0);

            //check if connection is even open
            if (bytes_recv < 1)
            {

                //there's nothing to do but wait for a new connection
                printf("Connection lost...\n");
                STATE = LISTENING;
            }
            else
            {

                printf("~~~~~From client [%s][%lu bytes]~~~~~\n", ipv4, strlen(buffer));
                printf("%s\n", buffer);
                printf("~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
                memset(buffer, 0, BUFFERLEN); //reset for next message
            }
        }
            else {}
    } //while

    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

    printf("Finished.\n");
    return 0;

} //main
