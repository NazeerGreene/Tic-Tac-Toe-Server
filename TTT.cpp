//TTT client
//IPv4 only

#include <iostream>
#include <array>
#include <string>
#include <random>
#include "helpers.h"

#define TTT_SERVER_PORT         "6767"

#define DEFAULT_PLAYER_CHAR     "X"
#define DEFAULT_OPPONENT_CHAR   "O"
#define DEFAULT_EMPTY_CHAR      "~"
#define BOARD_INDEX_OFFSET       1
//BOARD_INDEX_OFFSET is necessary to adjust user input
//into the proper array index
//user options [1...9] - BOARD_INDEX_OFFSET --> indexing [0..8]

#define ERROR   -1
#define SUCCESS  0

class TTT {
    //Basic 3x3 tic tac toe game; not scalable
    //Spaces enumerated from [0...8]
    //board characters refer to player, opponent, and empty space
    public:

    //default constructor
    TTT()
        : points(0), server_fd(0)
    {
        player_char = DEFAULT_PLAYER_CHAR;
        opp_char    = DEFAULT_EMPTY_CHAR;
        empty_space = DEFAULT_EMPTY_CHAR;

        board.fill(empty_space);

        std::cout << "Welcome, player " << player_char << "!\n" << std::endl;
    }

    //Constructor for player to define board decorations, each decoration length 1.
    TTT(const std::string player, const std::string opponent, const std::string empty_space_) 
        : points(0), server_fd(0) //0 is considered invalid
    {
        //board characters can't be empty or greater than one
        if (player.length() > 1 or player.empty()) 
            player_char = DEFAULT_PLAYER_CHAR;
        else
            player_char = player;

        if (opponent.length() > 1 or opponent.empty()) 
            opp_char = DEFAULT_OPPONENT_CHAR;
        else
            opp_char = opponent;
        
        if (empty_space_.length() > 1 or empty_space_.empty()) 
            empty_space = DEFAULT_EMPTY_CHAR;
        else
            empty_space = empty_space_;

        board.fill(empty_space);

        std::cout << "Welcome, player " << player_char << "!\n" << std::endl;
    }

    //The method to actually play the game.
    //It will draw the board and take control until
    //the number of turns reach 0 or the player
    //sends EOF.
    //Depends on engine to generate pseudo-random moves by 
    //the opponent.
    void game(std::default_random_engine & engine) 
    {
        //store winner character 
        //[options: player char, opponent char, none ("")]
        std::string winner{""};
        std::string out{""};

        out = "\nNew Game!\n";
        std::cout << "\n" << out << "\n";
        send_to_server(out);

        while (turns_left > 0) 
        {
            if(is_player_turn) 
            {
                std::cout << valid_spaces << std::endl;
                print_board(true);
                std::cout<< std::endl;

                //get user input; data validation already checked
                unsigned input = valid_input();
                //user called EOF, return to main; no points
                if (input == EOF) 
                {
                    std::cin.clear();
                    if (server_fd)   send_to_server(player_char + ": <Input: EOF>\n");
                    return;
                }

                //only advanced if space is valid; otherwise continue
                if (board.at(input) == empty_space) {
                    board.at(input) = player_char;
                    if (server_fd)   send_to_server(player_char + ": <Input: " + std::to_string(input) + ">\n");
                    is_player_turn = false;
                    turns_left--;
                }
            } else { //not player's turn
                //get next empty spot, random assignment
                while(1)
                {
                    //generates "move" in [0..8] 
                    auto i = engine() % 9;
                    //opponent attempts to occupy spot
                    if (board.at(i) == empty_space) {
                        board.at(i) = opp_char;
                        if (server_fd)   send_to_server(opp_char + ": <Input: " + std::to_string(i) + ">\n");
                        is_player_turn = true;
                        break;
                    }
                } //end of while
            } //end of conditional

            winner = check_for_win();
            //no winner
            if (winner == "") 
            {
                continue;
            }
            //if there is a winner...
            out = "\n\n~~~Player " + 
            (winner == player_char ? player_char : opp_char) + 
            " won!~~~\n";

            std::cout << out;
            send_to_server(out);

            print_board(true);
            points += (winner == player_char ? 5 : 0);
            clear_board();
            return;
        
        } //end of while

        //if turns reach 0, then no one wins
        out = "It's a draw!\n";
        std::cout << out;
        send_to_server(out);
        clear_board();
        return;
    }

    //draws board to std::cout
    void print_board(bool send_board = false)
    {
        std::string out{""};

        out = "-------------\n";
        for(unsigned int i = 0; i < 9; i += 3)
        {
            out += "| " + board.at(0 + i) + 
            " | " + board.at(1 + i) + " | " + board.at(2 + i) + " |\n";
        }
        out += "-------------\n";

        std::cout << out;

        if (send_board && server_fd)
            send_to_server(out);
    }

    //returns the points the user accumalated 
    //for each win
    unsigned get_points(void) 
    {
        return points;
    }

    //resets the board for a new game
    void clear_board(void) 
    {
        //reset to defaults
        board.fill(empty_space);
        turns_left      = 9;
        is_player_turn  = 1;
    }

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

    private:

    //-----------player-----------
    //accumalated points for each win
    unsigned points;

    //-----------board control-----------
    std::array<std::string, 9> board;
    bool is_player_turn     = 1;
    unsigned int turns_left = 9;

    //-----------helpers-----------
    const std::string valid_spaces = "1 2 3\n"
    "4 5 6\n"
    "7 8 9\n";

    //-----------board decorations-----------
    std::string player_char;
    std::string opp_char;
    std::string empty_space;

    //-----------networking------------------
    SOCKET server_fd; //0 is not valid

    //-----------helper functions-----------

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

    //check for win conditions in a traditional
    //3x3 Tic Tac Toe game
    std::string check_for_win()
    {
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

    //converts string to unsigned int, data validation checked
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

    //gets user input from std::in
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

}; //end of class