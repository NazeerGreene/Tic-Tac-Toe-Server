//TTT server -- 
//IPv4 only for now
//does not handle ctrl-D, use ctrl-C to terminate session

#include "TTTNET.hpp"        /* for networking purposes */
#include "TTT_Controls.hpp"  /* for the TTT game itself */
#include <iostream>
#include <string>
#include <array>

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
unsigned send_to_client(SOCKET client, enum SERVER_TO_CLIENT_TYPES type, std::string data="");

// BLOCKING -- wait for client to acknowledge last message 
// before sending the next
unsigned wait_for_ACK(SOCKET client);

int main(int argc, char * argv[]) 
{
    /* defaults for initiating the server */
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
//---------------------------- FINISHED INIT -----------------------------
//---------------------------- INITIALIZING THE GAME ---------------------------
    GAMESTATE game {
        .empty_space = DEFAULT_EMPTY_CHAR,
        .turns_left = 9
    };
    
    game.board.fill(game.empty_space);
    bool game_active = true; /* for the main game loop */

    CLIENT X {
        .fd = 0,
        .decorator = DEFAULT_X_CHAR
    };

    CLIENT O {
        .fd = 0,
        .decorator = DEFAULT_O_CHAR
    };

    std::array<CLIENT, 2> clients = { X, O };


    bool should_request_move  = true; /* "should request move" from the player */
    int  next_move_belongs_to = 0;    /* so we don't request a move from the wrong player */
    bool client_connection_closed = false;

    /*
     store winner character
     [options: player char, opponent char, empty space (no winner)]
    */
    char winner = game.empty_space;
    std::string text{""}; /* the instructions we'll send to the player */
    std::string preface{""};

    /* current player -- either 0 (X) or 1 (O) */
    unsigned i = 1;

    /* BLOCKING -- wait for two clients to connect */
    for(CLIENT& client: clients)
    {
        client.fd = wait_for_client(socket_listen, &master);
        if (client.fd == EOF) game_active = false;
        FD_SET(client.fd, &master);
        if (client.fd > max_socket)    max_socket = client.fd;
        
        /* So the game doesn't look frozen to the first person connected */
        if (client.decorator == DEFAULT_X_CHAR)
            send_to_client(client.fd, SERVER_TO_CLIENT_TYPES::SENDING_MSG,
                           "Waiting for another player...\n");
    }



//------------------------ FINISHED INITIALIZING THE GAME --------------------------

    while(game_active) // purpose: reading from stdin, clients; mananging the game
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
        i = !i; /* switch clients based on array indexing */
                
        /*
            For readability: player is the current client we are reading from,
            and opponent is the client's opponent (duh), so it's relative.
         */
        CLIENT& player   = clients.at(i);
        CLIENT& opponent = clients.at(!i);

        if ( next_move_belongs_to == i && should_request_move )
        {
            signed status = 0;
            
            if (game.turns_left == 9)
            {
                text  = player.decorator;
                text += opponent.decorator;
                if ( send_to_client(player.fd, SERVER_TO_CLIENT_TYPES::INIT, text) )
                    { client_connection_closed = true; }
                text.clear();
                text  = opponent.decorator;
                text += player.decorator;
                if ( send_to_client(opponent.fd, SERVER_TO_CLIENT_TYPES::INIT, text) )
                    { client_connection_closed = true; continue; } /* we want to make sure opponent switches to player */
                text.clear();
            }
            
            
            /* send a request to the player for a move */
            status = send_to_client(player.fd, SERVER_TO_CLIENT_TYPES::SENDING_BOARD, game.board.data());
            status = send_to_client(player.fd, SERVER_TO_CLIENT_TYPES::REQUESTING_MOVE);

            
            if (status == EOF)
            {
                client_connection_closed = true;
            } /* nothing else to process */

            
            /* notify opponent so the game doesn't look frozen */
            text  = "Waiting for ";
            text += player.decorator;
            text += " to make a move.\n";
            status = send_to_client(opponent.fd, SERVER_TO_CLIENT_TYPES::SENDING_BOARD, game.board.data());
            status = send_to_client(opponent.fd, SERVER_TO_CLIENT_TYPES::SENDING_MSG, text);

            if (status == EOF)
            {
                client_connection_closed = true;
                continue;
            } /* nothing else to process */
            
            should_request_move = false;
        } 

        if ( FD_ISSET(player.fd, &reads) )
        {
            TYPE_LENGTH_DATA client_msg;
            memset(&client_msg, 0, sizeof(TYPE_LENGTH_DATA)); // zero out
            
            /* we only want to look at the type and length first before any more operations on the sent data */
            ssize_t bytes_received = recv(player.fd, &client_msg, 2, 0);
            std::cout << "Received " << bytes_received << " from client " << player.decorator << " (fd: " << player.fd << ")\n";
            if ( client_msg.length ) bytes_received = recv(player.fd, client_msg.payload.msg, client_msg.length, 0);
            
            if ( bytes_received < 1) // connection lost
            {
                std::cout << "Connection closed by client (FD_ISSET).\n";
                client_connection_closed = true; /* reset game and get new client */
            }

            if ( client_msg.type == CLIENT_TO_SERVER_TYPES::SENDING_MOVE
                && next_move_belongs_to == i )
            {
                u_int8_t move = client_msg.payload.data;
                
                /* put player move onto board */
                bool valid_move = player_move(move, player.decorator, game);

                if (valid_move)
                { next_move_belongs_to = !next_move_belongs_to; /* switch turns */ }
                else 
                {
                    text = "PREVIOUS MOVE: " + std::to_string(move+1) + " WAS NOT A VALID MOVE.\n";
                    client_connection_closed = send_to_client(player.fd, SERVER_TO_CLIENT_TYPES::SENDING_MSG, text);
                }
                
                /* we will need a new move whether it's from the opponent or the player (because of a faulty previous move). */
                should_request_move = true;

            } // CLIENT_TO_SERVER_TYPES::RECEIVING_MOVE

            else if ( client_msg.type == CLIENT_TO_SERVER_TYPES::TERMINATED )
            {
                std::cout << "Received TERMINATED flag from client, closing connection with client.\n";
                send_to_client(player.fd, SERVER_TO_CLIENT_TYPES::SENDING_MSG, "Thank you for playing!\n");
                client_connection_closed = true; /* reset game and get new client */

            } // CLIENT_TO_SERVER_TYPES::TERMINATED
            
            else {} /* if we don't know how to interpret, throw away the message */

        } // FD_ISSET(client, &reads)


        if (client_connection_closed) /* if there's no client */
        {
            /* a fresh start */
            clear_board(game); /* restart game values: board, turns, player_turn = true */
            text.clear();

            next_move_belongs_to = !i; /* will toggle at top of loop */

            should_request_move = true;

            /* we no longer need this client */
            close(player.fd);
            FD_CLR(player.fd, &master);
            
            /* notify opponent of status */
            send_to_client(opponent.fd, SERVER_TO_CLIENT_TYPES::SENDING_MSG,
                           "\nPrevious player disconnected, waiting for a new player...\n");
            
            std::cout << "\n---> WAITING FOR CONNECTIONS... ";
            player.fd = wait_for_client(socket_listen, &master);
            if (player.fd == EOF) break; // EOF from admin (stdin)

            FD_SET(player.fd, &master);

            max_socket = ( player.fd > opponent.fd ? player.fd : opponent.fd );
            client_connection_closed = false;
            
            // send opponent game updates
            continue; /* in case admin wants to shut down and no point in checking win conditions */
        }

        winner = check_for_win(game);

        bool game_over = false;
        
        if (winner == game.empty_space && game.turns_left == 0) /* no winner and game over */
        {
            text = "\nIt's a draw!\n";
            game_over = true;
        }
        else if (winner != game.empty_space) /* there is a winner and game over */
        { 
            text  = "\n\n~~~Player ";
            text += winner;
            text += " won!~~~\n";
            
            game_over = true;            
        }

        if (game_over)
        {
            std::cout << text;   
            for(CLIENT i: clients) 
            {
                send_to_client(i.fd, SERVER_TO_CLIENT_TYPES::SENDING_MSG, text);
                send_to_client(i.fd, SERVER_TO_CLIENT_TYPES::SENDING_BOARD, game.board.data());
            }
            clear_board(game); // restart game
            text.clear();

            game_over = false;
        }

        //----------------------------- FINISHED THE GAME --------------------------------

    } //while

    exit:
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
                std::cin.ignore(100, '\n'); // flush any leftover input
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

unsigned send_to_client(SOCKET client_fd, enum SERVER_TO_CLIENT_TYPES type, std::string msg)
{
    static const std::array<std::string, SERVER_TO_CLIENT_TYPES::SIZE> packet_type_name =
    { "REQUESTING_MOVE", "SENDING_MSG", "SENDING_BOARD", "INIT" };
    
    TYPE_LENGTH_DATA packet;
    memset(&packet, 0, sizeof(TYPE_LENGTH_DATA));
    static int loop = 0; // for debugging

    /* so that the client knows what the server is requesting */
    packet.type = type;
    
    /* to reduce the size in case c++11 allocated more than necessary */
    msg.shrink_to_fit();

    unsigned long length = msg.length();

    if (length > 255)
    {
        std::cerr << "Error: MSG length too long ["
        << length << "]\n";

        return -2;
    }
    
    packet.length = length;
    
    // copying msg into packet.msg buffer
    for(unsigned long i = 0; i < length; i++)
        packet.payload.msg[i] = msg[i];

    /* for debugging */
    std::cout << "packet " << loop++ <<
    "\n[\n\t" << "type\t" << packet_type_name.at(packet.type - 1) <<
    "\n\t" << "length  " << std::to_string(length) <<
    "\n\tpayload \"" << msg << "\"\n]" << std::endl;

    /* send the data */
    ssize_t bytes_sent = send(client_fd, &packet, 2 + packet.length, 0);

    if (bytes_sent < 1)
    {
        std::cerr << "Lost connection with client" << std::endl;
        return EOF;
    }

    return 0; // SUCCESS
}
