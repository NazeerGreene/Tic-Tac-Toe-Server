//TTT server -- 
//IPv4 only for now
//does not handle ctrl-D, use ctrl-C to terminate session

#include "helpers.hpp"
#include <iostream>
#include <string>

#define PACKET_MSG_SIZE 50

// Game states for the server depending on the connection with the client
enum STATES { LISTENING, ACCEPTING, ESTABLISHED, LOST };

// types for communicating with the client (type-length-data)
enum SERVER_TO_CLIENT_TYPES { REQUEST_TAG = 1, REQUEST_MOVE };

// types to interpret messages from player (type-length-data)
enum CLIENT_TO_SERVER_TYPES { RECEIVING_TAG = 1, RECEIVING_MOVE, TERMINATED = 200 };

// create and bind socket to port
SOCKET bind_server_and_get_listen_socket(const char *port);

// blocking; wait to a client to connect, print out client info (IP addr)
// and then return
SOCKET wait_for_client(SOCKET listener);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa); // provided by Beej's Networking Guide

// request move from player, blocking
int32_t Request_Move(SOCKET client, std::string msg);


int main() 
{
//----------------------- INITIALIZING CONNECTION SETUP  ---------------------
    const char *port = "6767";

    SOCKET socket_listen = bind_server_and_get_listen_socket(port);
    SOCKET client = 0;

    //listening for incoming connections
    printf("Listening...\n");

    /* we only want one client at a time */
    if (listen(socket_listen, 0) < 0) 
    {
        std::cerr << "listen() failed. " << GETSOCKETERRNO() << '\n';
        exit(EXIT_FAILURE);
    }
   
    /*
        There should be at most 2 sockets to listen for:
        1. Client (for gameplay)
        2. Server stdin (from admin)
    */

    fd_set master;      // socket collection
    fd_set reads;       // as to not modify the original copy
    FD_ZERO(&master);   // zero out
    FD_SET(STDIN_FILENO, &master); // assumes 0 on most systems for stdin
    SOCKET max_socket = STDIN_FILENO;

//---------------------------- FINISHED INIT -----------------------------
    // now we listen for incoming connections
    STATES STATE = LISTENING;

    while(true) 
    {
        reads = master;
        struct timeval timeout { .tv_sec = 0 , .tv_usec = 500000 /*microseconds*/ }; // 0.5 seconds

        //we don't pass a timeout because we want select() to block until its ready
        if (select(max_socket+1, &reads, 0, 0, &timeout) < 0) 
        {
            std::cerr << "select() failed. " << GETSOCKETERRNO() << '\n';
            exit(EXIT_FAILURE);
        }

        /*
            This is where we handle standard input,
            like closing the server.
        */
       //------------ admin ----------------
       if (FD_ISSET(STDIN_FILENO, &reads))
       {
           std::string admin {};
           std::cin >> admin;
           // for administrators to safely exit
           if (std::cin.eof())
           {
               std::cin.clear();
               break; // from while
            }
       }
       //-------------------------------------

        if (!client) /* if there's no client */
        {
            client = wait_for_client(socket_listen);
            FD_SET(client, &master);
            if (client > max_socket)    max_socket = client;
            continue;
        }
        
        //-------------------------------- GAME STATES ---------------------------------
        
        int32_t player_move = Request_Move(client, "XXXXXXXXX");
        if (player_move == EOF) // connection lost
        {
            shutdown(client, 2); // send client the FIN flag
            close(client);       // we no longer need connection
            FD_CLR(client, &master); // we no longer need to check for
            max_socket = STDIN_FILENO;
            client = 0; // wait for a new client
        } else {
            std::cout << "Player Move: " << player_move << std::endl;
            //STATE = LOST;
        }

    //----------------------------- FINISHED GAME STATES ---------------------------------
    } //while

    std::cout << "Closing listening socket...\n";
    CLOSESOCKET(socket_listen);

    std::cout << "Finished.\n";
    return 0;

} //main

SOCKET bind_server_and_get_listen_socket(const char *listening_port)
// create the socket for the server to listen on and bind it 
// to the listening port (listening_port).
// returns the socket on success, exits on failure.
{
    //setting up our configurations
    std::cout << "Configuring local address...\n";
    struct addrinfo hints, *bind_address;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family     = AF_INET;     //IPv4
    hints.ai_socktype   = SOCK_STREAM; //TCP
    hints.ai_flags      = AI_PASSIVE;  //listen on every network interface

    getaddrinfo("127.0.0.1", listening_port, &hints, &bind_address);

    std::cout << "Creating socket...\n";
    SOCKET socket_listen;
    socket_listen = 
    socket(bind_address->ai_family,
            bind_address->ai_socktype, 
            bind_address->ai_protocol);

    if (!ISVALIDSOCKET(socket_listen)) 
    {
        std::cerr << "socket() failed. " << GETSOCKETERRNO() << '\n';
        exit(EXIT_FAILURE);
    }

    //binding socket to a local address
    std::cout << "Binding socket to local address...\n";
    if (bind(socket_listen,
                bind_address->ai_addr, 
                bind_address->ai_addrlen)) 
    {
        std::cerr << "bind() failed. " << GETSOCKETERRNO() << '\n';
        exit(EXIT_FAILURE);
    }

    //free linked list
    freeaddrinfo(bind_address);

    return socket_listen;
}


SOCKET wait_for_client(SOCKET listener)
{
     //-----------client information-------------
    #define BUFFERLEN 100

    struct sockaddr_in client_sockaddr; //IPv4
    socklen_t client_size = 0;
    SOCKET client = 0;
    char ipv4[INET_ADDRSTRLEN]; // for presentation addr
    memset(&ipv4, 0, INET_ADDRSTRLEN); //zero out
    
    //buffer to hold anything the client sends
    char buffer[BUFFERLEN];
    memset(&buffer, 0, BUFFERLEN);
    //-------------------------------------------

    while (!client)
    {
        std::cout << "Waiting for connections...\n";

        client = accept(listener, (struct sockaddr *)&client_sockaddr, &client_size);

        //check for failed connection
        if (client == -1)
        {
            std::cerr << "accept failed.";       
        }

        //covert network to presentation (easy read)
        inet_ntop(AF_INET, &(client_sockaddr.sin_addr), ipv4, INET_ADDRSTRLEN);
        std::cout << "server: got connection from " << ipv4 << '\n';
    } // while (!client)

    return client;
}

// for later use, adapting ipv4 and v6
// provided by Beej from Beej's Guide
void *get_in_addr(struct sockaddr *sa) 
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    else
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int32_t Request_Move(SOCKET client, std::string msg)
{
    TYPE_LENGTH_DATA server_msg;
    memset(&server_msg, 0, sizeof(server_msg)); // zero out

    static int loop = 0; // for debugging

    // so that the client knows what the 
    // server is requesting
    server_msg.type = SERVER_TO_CLIENT_TYPES::REQUEST_MOVE;

    // to reduce the size in case c++11 allocated more than necessary
    msg.shrink_to_fit();

    u_int8_t length =  msg.length();

    if (length > 255)
    {
        std::cerr << "Error: MSG length too long [" 
        << length << "]\n";

        return -2;
    }

    server_msg.length = length;

    // copying msg into packet.msg buffer
    for(u_int8_t i = 0; i < length; i++)    server_msg.payload[i] = msg[i];

    std::cout << "packet " << loop++ << "\n[\n\t" << "type\tREQUEST_MOVE\n\t" << "length  " << std::to_string(length) <<
    "\n\tpayload \"" << msg << "\"\n]" << std::endl;

    int bytes_sent = send(client, &server_msg, sizeof(server_msg), 0);

    // blocking -- we wait for client to return 

    TYPE_LENGTH_DATA client_msg;
    std::cout << "waiting for client...\n";
    int bytes_received = recv(client, &client_msg, sizeof(TYPE_LENGTH_DATA), 0);

    std::cout << "Message received: type(" << std::to_string(client_msg.type) << ") length(" << std::to_string(client_msg.length) << ")\n";

    if (bytes_received < 1 ) // connection closed
    {
        std::cout << "Lost connection with client\n";
        return EOF;
    }

    return static_cast<int32_t>(client_msg.payload[0]); // the move should be the first element
}
