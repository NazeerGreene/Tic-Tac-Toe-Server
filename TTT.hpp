//Basic 3x3 tic tac toe game; not scalable
//Spaces enumerated from [0...8]
//board characters refer to player, opponent, and empty space

#ifndef TTT_HPP
#define TTT_HPP

#include "helpers.hpp"
// Standard includes
#include <iostream>
#include <array> /* for the board */
#include <string>

#define DEFAULT_PLAYER_CHAR     "X"
#define DEFAULT_OPPONENT_CHAR   "O"
#define DEFAULT_EMPTY_CHAR      "~"
#define BOARD_INDEX_OFFSET       1
//BOARD_INDEX_OFFSET is necessary to adjust user input
//into the proper array index
//user options [1...9] - BOARD_INDEX_OFFSET --> indexing [0..8]

#define ERROR   -1
#define SUCCESS  0

typedef struct {
    //-------board decorations------
    std::string player_char;
    std::string opp_char;
    std::string empty_space;

    //----------controls-----------
    std::array<std::string, 9> board;
    bool is_player_turn;
    unsigned int turns_left; /* should be 9, 1 for each empty space */

} GAMESTATE;


//-----------helpers-----------
const std::string valid_spaces = 
    "1 2 3\n"
    "4 5 6\n"
    "7 8 9\n";

//-----------player-----------
//accumalated points for each win
//unsigned points;



/*
    Default board decorators for the player, the opponent, and the empty space.
    See definitions at top of header. Ex DEFAULT_PLAYER_CHAR
*/
void TTT_default_board_decorators(std::string& player_char, std::string& opp_char, std::string& empty_space);

/*
    Verifies if the characters are unique and of length 1.
*/
void TTT_verify_board_decorators(std::string& player, std::string& opponent, std::string& empty_space);

/*
    The method for the player to take one turn.
*/
bool player_move(unsigned move, GAMESTATE& game);

/*
    The method for a "robot" to take one turn.
*/
bool robot_move(unsigned move, GAMESTATE& game);

/*
    draws board to stdout
*/
void print_board(GAMESTATE& game);//(bool send_board = false)

/*
//returns the points the user accumalated 
//for each win
unsigned get_points(void) 
{
    return points;
}
*/

/*
    resets the board for a new game
*/
void clear_board(GAMESTATE& game);

/*
//establishes a connection with the server, optional
signed int establish_connection(const std::string address)
{
    std::cout << "\nAttempting to connect to server...";

    //creating hints
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM; //tcp
    hints.ai_family = AF_INET;      //IPv4
    struct addrinfo *server_addr;

    //attempt to connect (DNS)
    if (getaddrinfo(address.c_str(), TTT_SERVER_PORT, &hints, &server_addr)) 
    {
        std::cerr << "getaddrinfo() failed." << GETSOCKETERRNO() << "\n";
        return ERROR;
    } else {
        std::cout << "success.\n";
    }

    //creating the socket
    std::cout << "Creating socket...";
    SOCKET server_sock;
    server_sock = 
    socket(server_addr->ai_family,
            server_addr->ai_socktype, 
            server_addr->ai_protocol);
    if (!ISVALIDSOCKET(server_sock)) 
    {
        std::cerr << "socket() failed" << GETSOCKETERRNO() << "\n";
        return ERROR;
    } else {
        std::cout << "success.\n";
    }

    //initiate the TCP connection
    std::cout << "Connecting...";
    if (connect(server_sock,
        server_addr->ai_addr, 
        server_addr->ai_addrlen)) 
    {
        std::cerr << "connect() failed" << GETSOCKETERRNO() << "\n";
        return ERROR;
    } else {
        std::cout << "connected\n";
    }
    //we no longer need this information since the connection is established
    freeaddrinfo(server_addr);

    //finally we have a reliable socket to send the server data
    server_fd = server_sock;

    return SUCCESS;
}
*/

/*
//-----------networking------------------
SOCKET client_fd; //0 is not valid
*/

//-----------helper functions-----------
/*
//sending basic string data to server
signed int send_to_server(const std::string msg, bool verbose = 0)
{
    if (verbose)
    {
        std::cout << "Sending " << msg.length() << " bytes of data\n" <<
        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" << msg << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    }

    int bytes_sent = send(server_fd, msg.c_str(), msg.length(), 0);
    return SUCCESS;
}
*/

/*
    check for win conditions in a traditional 3x3 Tic Tac Toe game.
    returns the winning character
*/
std::string check_for_win(GAMESTATE& game);

/*
    converts string to unsigned int, data validation checked
*/
unsigned convert_to_unsigned(std::string s);

/*
    Continuously prompts the user for input and until 
    a positive integer is found or end-of-file
*/
unsigned valid_input();

#endif // TTT_HPP