#include "TTT_Controls.hpp"

void TTT_default_board_decorators(char * const x, char * const o, char * const empty_space)
{
    *x = DEFAULT_X_CHAR;
    *o = DEFAULT_O_CHAR;
    *empty_space = DEFAULT_EMPTY_CHAR;
}

void TTT_verify_board_decorators(char * const x, char * const o, char * const empty_space)
{
    if (*x == DEFAULT_O_CHAR ||
        *x == DEFAULT_EMPTY_CHAR) 

        *x = DEFAULT_X_CHAR;
    

    if (*o == *x || 
        *o == DEFAULT_X_CHAR || 
        *o == DEFAULT_EMPTY_CHAR)

        *o = DEFAULT_O_CHAR;
    
    
    if (*empty_space == *x || 
        *empty_space == *o || 
        *empty_space == DEFAULT_X_CHAR || 
        *empty_space == DEFAULT_O_CHAR) 

        *empty_space = DEFAULT_EMPTY_CHAR;
}

bool player_move(const unsigned move, const char decorator, GAMESTATE& game)
{
    //only advanced if space is valid; otherwise continue
    if (move > 8)   return false;
    
    if (game.board.at(move) == game.empty_space)
    {
        game.board.at(move) = decorator;
        game.turns_left--;
        return true;
    }

    std::cout << " << Invalid: Pick a new space >>\n";
    return false;
}

std::string format_board(const GAMESTATE& game)//(bool send_board = false)
{
    std::string out{""};

    out = "-------------\n";
    for(unsigned int i = 0; i < 9; i += 3)
    {
        out += "| ";
        out += game.board.at(0 + i);
        out += " | ";
        out += game.board.at(1 + i);
        out += " | ";
        out += game.board.at(2 + i);
        out += " |\n";
    }
    out += "-------------\n";

    return out;
}

void clear_board(GAMESTATE& game) 
{
    //reset to defaults
    game.board.fill(game.empty_space);
    game.turns_left = 9;
    //game.is_player_turn = true;
}

char check_for_win(const GAMESTATE& game)
{
    auto& board = game.board;
    const char empty_space = game.empty_space;

    //check rows
    for(unsigned i = 0; i < 9; i += 3) 
    {  
        /*  
            if even one empty space is found, then there's
            no point in checking the entire row;
            plus 3 empty spaces are technically a valid win,
            so we want to avoid that.
        */
        if (board.at(0+i) == empty_space) continue;

        /* check [0,1,2], [3,4,5], [6,7,8] */
        if( board.at(0+i) == board.at(1+i) && 
        board.at(1+i) == board.at(2+i) ) 
        return board.at(0+i);
    }

    // check columns
    for(unsigned i = 0; i < 3; i++) 
    {
        if (board.at(0+i) == empty_space) continue;

        //check [0,3,6], [1,4,7], [2,5,8]
        if( board.at(0+i) == board.at(3+i) && 
        board.at(3+i) == board.at(6+i) ) 
        return board.at(0+i);
    }

    /* check diagonal -- upper left to bottom right
        ex. o~~
            ~o~
            ~~o
    */
    if( board.at(0) == board.at(4) &&
    board.at(4) == board.at(8) &&
    board.at(8) != empty_space) 
    return board.at(0);

    /* check diagonal -- bottom left to upper right
        ex. ~~o
            ~o~
            o~~
    */
    if( board.at(6) == board.at(4) && 
    board.at(4) == board.at(2) &&
    board.at(2) != empty_space) 
    return board.at(6);

    //no one won
    return empty_space;
}
