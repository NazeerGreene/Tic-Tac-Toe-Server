/*
    Basic 3x3 tic tac toe game.
    Spaces enumerated from [0...8].
    Board characters refer to player X, player O, and empty space.
*/

#ifndef TTT_CONTROLS_HPP
#define TTT_CONTROLS_HPP

    /* Standard includes */
    #include <iostream>
    #include <string>
    #include <array> /* for the board */

    #define DEFAULT_X_CHAR     'X'
    #define DEFAULT_O_CHAR     'O'
    #define DEFAULT_EMPTY_CHAR '~'

    //-----------player-----------
    /*
        Represents a single player (client) in the game.
        There should be only 2 players for the entire game.
    */
    typedef struct {
        int  fd;            /* for the socket */
        char decorator;     /* traditionally 'x' or 'o', but can be other characters. */
    } CLIENT;

    typedef struct {
        //-------board decorations------
        char empty_space;

        //----------controls-----------
        std::array<char, 9> board;
        unsigned int turns_left; /* should be 9, 1 for each empty space */

    } GAMESTATE;

    /*
        Default board decorators for the player, the opponent, and the empty space.
        See definitions at top of header. Ex DEFAULT_PLAYER_CHAR
    */
    void TTT_default_board_decorators(char * const x, char * const o, char * const empty_space);

    /*
        Verifies if the characters are unique. If two characters are duplicates then those 
        characters will be set to their respective defaults (see header for details).

        The characters may not request another's default decorator. For example, x cannot be 
        DEFAULT_O_CHAR, otherwise x will be set to DEFAULT_X_CHAR.
    */
    void TTT_verify_board_decorators(char * const x, char * const o, char * const empty_space);

    /*
        Checks that the spot on the board the player wants to fill.
            is empty : fills the spot with decorator and returns true; decrements turns_left.
            not empty: returns false.
    */
    bool player_move(const unsigned move, const char decorator, GAMESTATE& game);

    /*
        Resets the board for a new game, and the turns left in the game.
    */
    void clear_board(GAMESTATE& game);

    /*
        Check for win conditions in a traditional 3x3 Tic Tac Toe game.
        Returns the winning character or empty_space for no winner.
    */
    char check_for_win(const GAMESTATE& game);

#endif // TTT_CONTROLS_HPP
