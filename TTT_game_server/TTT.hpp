//Basic 3x3 tic tac toe game; not scalable
//Spaces enumerated from [0...8]
//board characters refer to player, opponent, and empty space

#ifndef TTT_HPP
#define TTT_HPP

    // Standard includes
    #include <iostream>
    #include <string>
    #include <array> /* for the board */

    #define DEFAULT_PLAYER_CHAR     "X"
    #define DEFAULT_OPPONENT_CHAR   "O"
    #define DEFAULT_EMPTY_CHAR      "~"

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
        returns a string of the board in traditional 3x3 frame
    */
    std::string format_board(const GAMESTATE& game);//(bool send_board = false)

    /*
        resets the board for a new game
    */
    void clear_board(GAMESTATE& game);

    /*
        check for win conditions in a traditional 3x3 Tic Tac Toe game.
        returns the winning character
    */
    std::string check_for_win(GAMESTATE& game);

#endif // TTT_HPP