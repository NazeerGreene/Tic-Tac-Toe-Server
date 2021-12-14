//TTT server -- 
//IPv4 only for now
//does not handle ctrl-D, use ctrl-C to terminate session

#include "TTTNET.hpp"        /* for networking purposes */
#include "TTT_Controls.hpp"  /* for the TTT game itself */
#include <iostream>
#include <string>
#include <random>      /* for the robot (opponent) */

#define FORGET(x, y)   close(x); FD_CLR(x, y); /* a shortcut for erasing clients: x (client socket), y (fd_set) */


// types for communicating with the client (type-length-data)
enum SERVER_TO_CLIENT_TYPES { REQUEST_TAG = 1, REQUEST_MOVE, SEND_MSG };

// types to interpret messages from client (type-length-data)
enum CLIENT_TO_SERVER_TYPES { RECEIVING_TAG = 1, RECEIVING_MOVE, ACK, TERMINATED = 200 };

// create and bind socket to local port; exits on failure;
// purpose: to generate socket for listening
SOCKET bind_server_and_get_listen_socket(const char * ip, const char *port);

// wait for a client to connect, blocking;
// adds listener socket to fd_set;
// print out client info (IP addr) and then return client file desc.
SOCKET wait_for_client(SOCKET listener, fd_set * ttt_set);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa); // provided by Beej's Networking Guide

// request move from player; blocking
// -2   message too long, must be less than 256 characters
// EOF  connection with client closed
signed Request_Move(SOCKET client, std::string msg);

// send a message to the client; non-blocking; 0 on success
signed send_msg_to_client(SOCKET client, std::string msg);

// BLOCKING -- wait for client to acknowledge last message 
// before sending the next
unsigned wait_for_ACK(SOCKET client);

int main() 
{
    // defaults for initiating the server
    std::string TTT_ip {"127.0.0.1"};
    std::string TTT_port {"6767"};

    if (argc > 1) // if other ip or port specified
    {
        TTT_ip = argv[1]; // should be alterantive ip

        if(argc > 2) TTT_port = argv[2]; // should be alternative port
    }

//----------------------- INITIALIZING CONNECTION SETUP  ---------------------
    SOCKET socket_listen = bind_server_and_get_listen_socket( TTT_ip.c_str(), TTT_port.c_str() );
   
    /*
        There should be at most 3 sockets to listen for:
        1. stdin (from admin)
        2. client_x 
        3. client_o
    */

    fd_set master;      // socket collection
    fd_set reads;       // to not modify the original copy
    FD_ZERO(&master);   // zero out
    FD_SET(STDIN_FILENO, &master); // assumes 0 on most systems for stdin
    SOCKET max_socket = STDIN_FILENO;

    //CLIENT X, O;
    //memset(X, 0, sizeof(CLIENT)); // zero out
    //memset(O, 0, sizeof(CLIENT)); // zero out

//---------------------------- FINISHED INIT -----------------------------
//---------------------------- INITIALIZING THE GAME ---------------------------
    GAMESTATE game {
        .empty_space = '',
        .turns_left = 9
    };

    CLIENT X {
        .fd = 0,
        .decorator = DEFAULT_X_CHAR,
        .is_player_turn = true,
        .did_request    = false
    };

    CLIENT O {
        .fd = 0,
        .decorator = DEFAULT_O_CHAR,
        .is_player_turn = false,
        .did_request    = true  // we don't need to send a request until X makes the first move
    };

    std::array<CLIENT, 2> clients = { X, O };

    // initialize the board decorators with default characters
    //TTT_default_board_decorators(game.player_char, game.opp_char, game.empty_space);

    bool did_request = false; /* "did request" a move from the player */
    bool client_connection_closed = false;
    
    // next 2 lines from https://zhang-xiao-mu.blog/2019/11/02/random-engine-in-modern-c/
    // seed + engine necessary to produce pseudo-random numbers for robot player
    //unsigned int seed = std::chrono::steady_clock::now().time_since_epoch().count();
    //std::default_random_engine engine(seed);

    // store winner character 
    // [options: player char, opponent char, empty space (no winner)]
    char winner = game.empty_space;
    std::string text{""}; /* the instructions we'll send to the player */

    /* BLOCKING -- wait for two clients to connect */
    for(CLIENT client: clients)
    {
        client = wait_for_client(socket_listen, &master);
        if (client == EOF) break;
        FD_SET(client, &master);
        if (client > max_socket)    max_socket = client;
    }
//------------------------ FINISHED INITIALIZING THE GAME --------------------------

    while(true) // purpose: reading from stdin, clients; mananging the game
    {
        FD_COPY(&master, &reads);
        struct timeval timeout { .tv_sec = 0 , .tv_usec = 200000 /*microseconds*/ }; // 0.5 seconds

        // we pass a timeout because we want don't select() to block
        if (select(max_socket+1, &reads, 0, 0, &timeout) < 0) 
        {
            std::cerr << "select() failed. " << GETSOCKETERRNO() << ' ';
            perror("");
            std::cout << std::endl;
            exit(EXIT_FAILURE);
        }

       //------------ admin ----------------
        /*
            This is where we handle standard input,
            like closing the server.
        */
       if (FD_ISSET(STDIN_FILENO, &reads))
       {
            if(getchar() == EOF) break;
            else 
            {
                std::cin.ignore(1000, '\n'); // flush any leftover input
                std::cin.clear(); // clear any bad conditions set by std::cin
            }
       }
       //-------------------------------------

        
        
        //----------------------------------- GAME  -------------------------------------

        
        for(size_t i = 0; i < 2; i++)
        {
            if (game.turns_left == 9) // new game
                text = "\nNew Game!\n";


            if ( FD_ISSET(clients.at(i).fd, &reads) )
            {
                TYPE_LENGTH_DATA client_msg;
                memset(&client_msg, 0, sizeof(TYPE_LENGTH_DATA)); // zero out
                
                // we only want to look at the type and length first before any more operations on the sent data
                ssize_t bytes_received = recv(client, &client_msg, 2, 0);
                std::cout << "Received: " << bytes_received << " from client" << std::endl;

                if (bytes_received < 1) // connection lost
                {
                    std::cout << "Connection closed by client (FD_ISSET).\n";
                    client_connection_closed = true; /* reset game and get new client */
                }

                if ( client_msg.type == CLIENT_TO_SERVER_TYPES::RECEIVING_MOVE 
                    && game.is_player_turn )
                {
                    u_int8_t move = 0;
                    ssize_t bytes_received = recv(client, &move, 1, 0);
                    
                    if (bytes_received < 1) // connection lost
                    {
                        std::cout << "Connection closed by client (RECEIVING_MOVE).\n";
                        client_connection_closed = true; /* reset game and get new client */
                    }

                    bool valid_move = player_move(move, game); // put player move onto board

                    if (valid_move)
                    {
                        // switch turns
                        clients.at(i).is_player_turn    = false; // no longer current client's turn
                        clients.at(!i).is_player_turn   = true;  // now it's the opponent's turn
                        clients.at(i).did_request       = true;  // there's no need to request a new move
                        clients.at(!i).did_request      = false; // we need the opponent to send a new move
                    }
                    else 
                    {
                        clients.at(i).did_request = false; // we need to request for a new move
                        text += "Previous move: " + std::to_string(move) + " was not a valid move.\n";
                    }

                } // CLIENT_TO_SERVER_TYPES::RECEIVING_MOVE

                else if ( client_msg.type == CLIENT_TO_SERVER_TYPES::TERMINATED )
                {
                    std::cout << "Recieved TERMINATED flag from client, closing connection with client.\n";
                    //shutdown(client, 2); // send client the FIN flag
                    client_connection_closed = true; /* reset game and get new client */

                } // CLIENT_TO_SERVER_TYPES::TERMINATED
                
                else { // if we don't know how to interpret, throw away the message
                    
                    char garbage[256];
                    ssize_t bytes_received = recv(client, &garbage, client_msg.length, 0);
                    
                    if (bytes_received < 1) // connection lost
                    {
                        std::cout << "Connection closed by client (garbage).\n";
                        client_connection_closed = true; /* reset game and get new client */
                    }
                } // else

            } // FD_ISSET(client, &reads)


            if (client_connection_closed) /* if there's no client */
            {
                // a fresh start
                clear_board(game); /* restart game values: board, turns, player_turn = true */
                text.clear();

                clients.at(0).is_player_turn = false; // start with X for a first move
                clients.at(1).is_player_turn = true;  // O will not make the first move

                clients.at(0).did_request  = true;  // we need to alert X of the first move
                clients.at(1).did_request  = false; // no need to send a move to O

                SOCKET client = clients.at(i).fd;

                // we no longer need this client
                close(client);
                FD_CLR(client, &master);
                
                std::cout << "\n---> WAITING FOR CONNECTIONS... ";
                client = wait_for_client(socket_listen, &master);
                if (client == EOF) break; // EOF from admin (stdin)

                FD_SET(client, &master);
                clients.at(i).fd = client;

                max_socket = ( clients.at(0).fd > clients.at(1).fd ? clients.at(0).fd : clients.at(1).fd );
                client_connection_closed = false;
                continue; /* in case admin wants to shut down */
            }


            if ( !clients.at(i).did_request )
            {
                text += valid_spaces + format_board(game); // output for the player
                signed status = Request_Move(client, text);
                text.clear();

                clients.at(i).did_request = true;
            } 

            winner = check_for_win(game);

            bool game_over = false;
            
            if (winner == game.empty_space && game.turns_left == 0) // no winner and game over
            {
                text = "It's a draw!\n" + format_board(game);
                
                game_over = true;
                
            } 
            else if (winner != game.empty_space) // there is a winner and game over
            { 
                text = "\n\n~~~Player " + 
                (winner == game.player_char ? game.player_char : game.opp_char) + 
                " won!~~~\n" + format_board(game);

                game_over = true;            
            }

            if (game_over)
            {
                std::cout << text;   
                for(CLIENT i: clients) 
                { send_msg_to_client(client, text); }
                clear_board(game); // restart game
                text.clear();

                game_over = false;
            }

        } // for

        //----------------------------- FINISHED THE GAME --------------------------------

    } //while

    std::cout << "Closing listening socket...\n";
    CLOSESOCKET(socket_listen);
    std::cout << "Closing clients' sockets...\n";

    for(CLIENT client: clients)
    {
        if(client.fd)
        {
            CLOSESOCKET(client.fd);
        }
    }
    
    std::cout << "Finished.\n";
    return 0;

} //main

SOCKET bind_server_and_get_listen_socket(const char * ip, const char *port)
{
    //setting up our configurations
    std::cout << "Configuring local address...\n";
    struct addrinfo hints, *bind_address;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family     = AF_INET;     //IPv4
    hints.ai_socktype   = SOCK_STREAM; //TCP
    hints.ai_flags      = AI_PASSIVE;  //listen on every network interface

    getaddrinfo(ip, port, &hints, &bind_address);

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

    int enabled = 1;
    /* Enable address reuse */
    if ( setsockopt( socket_listen, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(int) ) < 0)
    {
        std::cerr << "setsockopt() failed. " << GETSOCKETERRNO() << '\n';
        exit(EXIT_FAILURE);
    }

    // binding socket to a local address
    std::cout << "Binding socket to local address...\n";
    if (bind(socket_listen,
                bind_address->ai_addr, 
                bind_address->ai_addrlen)) 
    {
        std::cerr << "bind() failed. " << GETSOCKETERRNO() << '\n';
        exit(EXIT_FAILURE);
    }

    // free linked list
    freeaddrinfo(bind_address);

    // listening for incoming connections
    std::cout << "Listening for a new client...\n";

    // we only want one client at a time
    if (listen(socket_listen, 2) < 0) 
    {
        std::cerr << "listen() failed. " << GETSOCKETERRNO() << '\n';
        exit(EXIT_FAILURE);
    }

    return socket_listen;
}


SOCKET wait_for_client(SOCKET listener, fd_set * ttt_set)
{
    fd_set reads;       // to not modify the original copy
    FD_SET(listener, ttt_set);
    SOCKET max_socket = listener;

     //-----------client information-------------
    #define BUFFERLEN 100

    struct sockaddr_in client_sockaddr; //IPv4
    socklen_t client_size = 0;
    SOCKET client = 0;
    char ipv4[INET_ADDRSTRLEN];        // for presentation addr
    memset(&ipv4, 0, INET_ADDRSTRLEN); //zero out
    
    //buffer to hold anything the client sends
    char buffer[BUFFERLEN];
    memset(&buffer, 0, BUFFERLEN);
    //-------------------------------------------

    while (!client)
    {
        FD_COPY(ttt_set, &reads);
        struct timeval timeout { .tv_sec = 0 , .tv_usec = 200000 /*microseconds*/ }; // 0.5 seconds

        // we pass a timeout because we want don't select() to block
        if (select(max_socket+1, &reads, 0, 0, &timeout) < 0) 
        {
            std::cerr << "select() failed. " << GETSOCKETERRNO() << ' ';
            perror("");
            std::cout << std::endl;
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(STDIN_FILENO, &reads))
        {
            if(getchar() == EOF) return EOF;
            else 
            {
                std::cin.ignore(1000, '\n'); // flush any leftover input
                std::cin.clear(); // clear any bad conditions set by std::cin
            }
        }

        if(FD_ISSET(listener, &reads))
        {
            client = accept(listener, (struct sockaddr *)&client_sockaddr, &client_size);

            //check for failed connection
            if (client == -1)
            {
                std::cerr << "accept failed.";
                client = 0;
                continue;   
            }

            //covert network to presentation (easy read)
            inet_ntop(AF_INET, &(client_sockaddr.sin_addr), ipv4, INET_ADDRSTRLEN);
            std::cout << "NEW CONNECTION FROM " << ipv4 << '\n';
        }
        
    } // while (!client)

    FD_CLR(listener, ttt_set); // we no longer need to listen for it

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

signed Request_Move(SOCKET client, std::string msg)
{
    TYPE_LENGTH_DATA server_msg;
    memset(&server_msg, 0, sizeof(server_msg)); // zero out

    static int loop = 0; // for debugging

    // so that the client knows what the 
    // server is requesting
    server_msg.type = SERVER_TO_CLIENT_TYPES::REQUEST_MOVE;

    signed status = send_msg_to_client(client, msg);
    if (status) return status;

    std::cout << "packet " << loop++ << "\n[\n\t" << "type\tREQUEST_MOVE\n]";

    int bytes_sent = send(client, &server_msg, 2, 0);

    if (bytes_sent < 1)
    {
        std::cerr << " LOST CONNECTION WITH CLIENT\n";
        return EOF;
    }

    return 0;
}

signed send_msg_to_client(SOCKET client, std::string msg)
{
    if (msg == "") return 0; // nothing to send
    
    TYPE_LENGTH_DATA server_msg;
    memset(&server_msg, 0, sizeof(server_msg)); // zero out

    static int loop = 0; // for debugging

    // so that the client knows what the 
    // server is requesting
    server_msg.type = SERVER_TO_CLIENT_TYPES::SEND_MSG;

    // to reduce the size in case c++11 allocated more than necessary
    msg.shrink_to_fit();

    u_int8_t length = msg.length();

    if (length > 255)
    {
        std::cerr << "Error: MSG length too long [" 
        << length << "]\n";

        return -2;
    }

    server_msg.length = length;

    // copying msg into packet.msg buffer
    for(u_int8_t i = 0; i < length; i++)    server_msg.payload[i] = msg[i];

    std::cout << "packet " << loop++ << "\n[\n\t" << "type\tSEND_MSG\n\t" << "length  " << std::to_string(length) <<
    "\n\tpayload \"" << msg << "\"\n]" << std::endl;

    // send a minimum of type-length, and then size of data
    int bytes_sent = send(client, &server_msg, 2 + length, 0);

    if (bytes_sent < 1)
    {
        std::cerr << "LOST CONNECTION WITH CLIENT\n";
        return EOF;
    }

    return 0;
}

