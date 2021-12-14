#include "TTTNET.hpp"
#include <iostream>

// so we know how to interpret server messages (type-length-data)
enum SERVER_TO_CLIENT_TYPES {REQUESTING_TAG = 1, REQUESTING_MOVE, SENDING_MSG };

// so we can send our messages to the server (type-length-data)
enum CLIENT_TO_SERVER_TYPES {SEND_TAG = 1, SEND_MOVE, ACK, TERMINATED = 200 };

// call to connect to TTT server given a ip and port.
// exits program upon failure to create a socket or connect to server.
SOCKET connect_to_ttt_server(std::string ip, std::string port);

// to send a single packet to the server
// big-endian: function will convert the move to network order
unsigned send_to_server(SOCKET server, enum CLIENT_TO_SERVER_TYPES type, u_int32_t data);

// to ensure user enters a valid digit [1-9]
unsigned convert_to_unsigned(std::string s);

// get user input from stdin, continuously ask for board position
// until int is entered, or EOF; [lower...upper] inclusive
u_int32_t get_board_position(unsigned lower, unsigned upper);

int main(int argc, char * argv[]) 
{
    // defaults for connecting to the server
    std::string TTT_ip {"127.0.0.1"};
    std::string TTT_port {"6767"};

    if (argc > 1) // if other ip or port specified
    {
        TTT_ip = argv[1]; // should be alterantive ip
        if(argc > 2)
            TTT_port = argv[2]; // should be alternative port
    }

    SOCKET server = connect_to_ttt_server(TTT_ip, TTT_port);

    unsigned move_from_player = 0;

    /*
        There should be at most 2 sockets to listen for:
        1. server
        2. stdin (player)
    */

   fd_set master;      // socket collection
   fd_set reads;       // for checking conditions -- copy of master
   FD_ZERO(&master);   // zero out
   FD_SET(STDIN_FILENO, &master); // assumes 0 on most systems for stdin
   FD_SET(server, &master); // add connection to server
   SOCKET max_socket = server;
   
   std::cout << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~ Let's Play! ~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
   while(true)
    {    
        reads = master;
        struct timeval timeout { .tv_sec = 0 , .tv_usec = 500000 /*microseconds*/ }; // 0.5 seconds

        //std::cout << "Before select()\n";
        if (select(max_socket+1, &reads, 0, 0, &timeout) < 0) 
        {
            std::cerr << "select() failed. " << GETSOCKETERRNO() << '\n';
            exit(EXIT_FAILURE);
        }
        //std::cout << "After select()\n";

        // safe exit by player
        if (FD_ISSET(STDIN_FILENO, &reads))
        {
            if(getchar() == EOF) break;
            else 
            {
                std::cin.ignore(1000, '\n'); // flush any leftover input
                std::cin.clear(); // clear any bad conditions set by std::cin
            }
        }

        // wait until server is ready 
        if (FD_ISSET(server, &reads)) 
        {
            //std::cout << "server ready for data\n";

            TYPE_LENGTH_DATA server_msg;
            memset(&server_msg, 0, sizeof(TYPE_LENGTH_DATA)); // zero out
            
            // we only want to look at the type and length first before any more operations on the sent data
            if ( recv(server, &server_msg, 2, 0) < 1) // connection lost
            {
                std::cout << "Connection closed by server.\n";
                break; // from loop, end program
            }

            // if the server is ready for a move
            if (server_msg.type == SERVER_TO_CLIENT_TYPES::REQUESTING_MOVE)
            {
                move_from_player = get_board_position(1,9);   // then get move from player
                
                if (move_from_player == EOF)
                {
                    send_to_server(server, CLIENT_TO_SERVER_TYPES::TERMINATED, 0); // send formal request to close game
                    continue;
                }
                else if
                    ( send_to_server(server, CLIENT_TO_SERVER_TYPES::SEND_MOVE , move_from_player-1) ) break; // break from game loop, end program, connection lost
                //user options [1...9] - 1 --> indexing [0..8]
            }

            else if (server_msg.type == SERVER_TO_CLIENT_TYPES::SENDING_MSG)
            {
                char buff[256];
                memset(buff, 0, 256); // zero out
                // guaranteed null termination since TYPE_LENGTH_DATA::length < 256

                ssize_t bytes_received = recv(server, &buff, server_msg.length, 0);
                
                if (bytes_received < 1) // connection lost
                {
                    std::cout << "Connection closed by server.\n";
                    break; // from loop, end program
                }

                std::cout << buff << std::endl; // display message from server (typically TTT board)
            }
                        
            else { // if we don't know how to interpret, throw away the message
                
                char garbage[256];
                ssize_t bytes_received = recv(server, &garbage, server_msg.length, 0);
                
                if (bytes_received < 1) // connection lost
                {
                    std::cout << "Connection closed by server.\n";
                    break; // from loop, end program
                }
                
            } // else

        } // FD_ISSET(server, &reads)
    } // while

    std::cout << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~ Finished ~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

    std::cout << "Closing server socket...\n";
    CLOSESOCKET(server);

    std::cout << "Thank you, game finished." << std::endl;

    return 0;
}

SOCKET connect_to_ttt_server(std::string ip, std::string port)
{
    std::cout << "Configuring Tic Tac Toe remote address...\n";

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM; //tcp
    hints.ai_family   = AF_INET;     // IPv4
    
    struct addrinfo *server_list;
   
    if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &server_list)) 
    {
        std::cerr << "getaddrinfo() failed. " << GETSOCKETERRNO() << '\n';
        exit(EXIT_FAILURE);
    }

    //not necessary but we print out the remote address for debugging
    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(server_list->ai_addr, server_list->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    std::cout << address_buffer << ' ' << service_buffer << '\n';

    //Create the socket to connect to server
    std::cout << "Creating socket...\n";

    SOCKET server;
    server = socket(server_list->ai_family,
                server_list->ai_socktype, 
                server_list->ai_protocol);
    if (!ISVALIDSOCKET(server)) 
    {
        std::cerr << "socket() failed. " << GETSOCKETERRNO() << '\n';
        exit(EXIT_FAILURE);
    }

    //connect associates a socket with a remote address and intiates
    //the TCP connection
    std::cout << "Connecting to Tic Tac Toe server... ";
    if (connect(server,
                server_list->ai_addr, 
                server_list->ai_addrlen)) 
    {
        std::cerr << "connect() failed. " << GETSOCKETERRNO() << '\n';
        exit(EXIT_FAILURE);
    }

    //we no longer need this information since the connection is established
    freeaddrinfo(server_list);

    //finally ready for user input from the terminal
    std::cout << "Connected.\n";

    return server;
}

unsigned send_to_server(SOCKET server, enum CLIENT_TO_SERVER_TYPES type, u_int32_t data)
{
    TYPE_LENGTH_DATA packet;
    memset(&packet, 0, sizeof(packet));

    /* so that the client knows what the server is requesting */
    
    switch (type) {
        case CLIENT_TO_SERVER_TYPES::SEND_MOVE :
            packet.type = CLIENT_TO_SERVER_TYPES::SEND_MOVE;
            packet.length = 1; // we only need a byte to send the move
            packet.payload[0] = data & 0xff;
            // it's safe to cut off any data since we don't need more than a byte to send the move
            break;
        case CLIENT_TO_SERVER_TYPES::TERMINATED :
            packet.type = CLIENT_TO_SERVER_TYPES::TERMINATED;
            packet.length = 0;
            break;
        default:
            packet.type = CLIENT_TO_SERVER_TYPES::ACK;
            packet.length = 0;
            break;
    }

    //std::cout << "packet\n[\n\t" << "type\tSEND_MOVE\n\t" << "length  " << std::to_string(packet.length) <<
    //"\n\tpayload " << char(packet.payload[0] + '0') << "\n]" << std::endl;

    // send the data
    ssize_t bytes_sent = send(server, &packet, 2 + packet.length, 0);

    if (bytes_sent < 1)
    {
        std::cerr << "Lost connection with server" << std::endl;
        return EOF;
    }

    return 0; // SUCCESS
}

unsigned convert_to_unsigned(std::string s) 
{
    unsigned d = 0;

    for(char c: s) 
        {
            if (c < '0' or c > '9')
                {
                    return EOF;
                }
            
            d *= 10;
            d += (c - '0');            
        }

    return d;
}

u_int32_t get_board_position(unsigned lower, unsigned upper)
{
    std::string input = "";
    unsigned dig = 0;
    while(1) 
    {
        std::cout << "Choose a space >> ";
        std::getline(std::cin, input);

        if (std::cin.eof())
        { 
            std::cout << "<Input: EOF>" << "\n";
            return EOF;
        }

        if (input == "exit" || input == "close")
        {
            std::cout << "Closing the connection with the server.\n";
            return EOF:
        }

        dig = convert_to_unsigned(input);

        if (lower <= dig and dig <= upper)
            return dig;
        else
        {
            std::cout << "Only [" << lower << "..." << upper << "]\n\n";
            std::cin.clear();
        }
    }
}
