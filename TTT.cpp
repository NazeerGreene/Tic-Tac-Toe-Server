#include "TTT.hpp"


void TTT_default_board_decorators(std::string& player_char, std::string& opp_char, std::string& empty_space)
{
    player_char = DEFAULT_PLAYER_CHAR;
    opp_char    = DEFAULT_OPPONENT_CHAR;
    empty_space = DEFAULT_EMPTY_CHAR;
}

void TTT_verify_board_decorators(std::string& player, std::string& opponent, std::string& empty_space) 
{
    if (player.length() != 1 || 
        player == DEFAULT_OPPONENT_CHAR ||
        player == DEFAULT_EMPTY_CHAR) 

        player = DEFAULT_PLAYER_CHAR;
    

    if (opponent.length() != 1 || 
        opponent == player || 
        opponent == DEFAULT_PLAYER_CHAR || 
        opponent == DEFAULT_EMPTY_CHAR)

        opponent = DEFAULT_OPPONENT_CHAR;
    
    
    if (empty_space.length() != 1 || 
        empty_space == player || 
        empty_space == opponent || 
        empty_space == DEFAULT_PLAYER_CHAR || 
        empty_space == DEFAULT_OPPONENT_CHAR) 

        empty_space = DEFAULT_EMPTY_CHAR;
}

bool player_move(unsigned move, GAMESTATE& game)
{
    //only advanced if space is valid; otherwise continue
    if (game.board.at(move) == game.empty_space)
    {
        game.board.at(move) = game.player_char;
        game.turns_left--;
        game.is_player_turn = false;
        return true;
        //if (server_fd)   send_to_server(player_char + ": <Input: " + std::to_string(input) + ">\n");
        //is_player_turn = false;
        //turns_left--;
    }
    std::cout << " << Invalid: Pick a new space >>\n";
    return false;
}

bool robot_move(unsigned move, GAMESTATE& game)
{
    if (game.board.at(move) == game.empty_space) 
    {
        std::cout << "robot made a valid move at " << move << '\n';
        game.board.at(move) = game.opp_char;
        game.is_player_turn = true;
        return true;
        //if (server_fd)   send_to_server(opp_char + ": <Input: " + std::to_string(i) + ">\n");
        //is_player_turn = true;
        //break;
    }

    return false;
}

void print_board(GAMESTATE& game)//(bool send_board = false)
{
    std::string out{""};

    out = "-------------\n";
    for(unsigned int i = 0; i < 9; i += 3)
    {
        out += "| " + game.board.at(0 + i) + 
        " | " + game.board.at(1 + i) + " | " + game.board.at(2 + i) + " |\n";
    }
    out += "-------------\n";

    std::cout << out;

    //if (send_board && server_fd)
        //send_to_server(out);
}

void clear_board(GAMESTATE& game) 
{
    //reset to defaults
    game.board.fill(game.empty_space);
    game.turns_left = 9;
    //is_player_turn  = 1;
}

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

std::string check_for_win(GAMESTATE& game)
{
    auto& board = game.board;
    const std::string empty_space = game.empty_space;

    //check rows
    for(unsigned i = 0; i < 9; i += 3) 
    {  
        //if even one empty space is found, then there's
        //no point in checking the entire row;
        //plus 3 empty spaces are technically a valid win,
        //so we want to avoid that.
        if (board.at(0+i) == empty_space) continue;

        //check [0,1,2], [3,4,5], [6,7,8]
        if( board.at(0+i) == board.at(1+i) && 
        board.at(1+i) == board.at(2+i) ) 
        return board.at(0+i);
    }

    //check columns
    for(unsigned i = 0; i < 3; i++) 
    {
        if (board.at(0+i) == empty_space) continue;

        //check [0,3,6], [1,4,7], [2,5,8]
        if( board.at(0+i) == board.at(3+i) && 
        board.at(3+i) == board.at(6+i) ) 
        return board.at(0+i);
    }

    //check diagonal -- upper left to bottom right
    //ex. oxx
    //    xox
    //    xxo
    if( board.at(0) == board.at(4) &&
    board.at(4) == board.at(8) &&
    board.at(8) != empty_space) 
    return board.at(0);

    //check diagonal -- bottom left to upper right
    //ex. xxo
    //    xox
    //    oxx
    if( board.at(6) == board.at(4) && 
    board.at(4) == board.at(2) &&
    board.at(2) != empty_space) 
    return board.at(6);

    //no one won
    return "";
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

unsigned valid_input()
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

        dig = convert_to_unsigned(input);

        //adjust to work with board positions
        dig -= BOARD_INDEX_OFFSET;

        if (dig < 9)
            return dig;
        else
        {
            std::cout << "Only [1...9]\n\n";
            std::cin.clear();
        }
    }
}