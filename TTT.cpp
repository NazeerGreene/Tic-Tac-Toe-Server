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

std::string format_board(const GAMESTATE& game)//(bool send_board = false)
{
    std::string out{""};

    out = "-------------\n";
    for(unsigned int i = 0; i < 9; i += 3)
    {
        out += "| " + game.board.at(0 + i) + 
        " | " + game.board.at(1 + i) + " | " + game.board.at(2 + i) + " |\n";
    }
    out += "-------------\n";

    return out;
}

void clear_board(GAMESTATE& game) 
{
    //reset to defaults
    game.board.fill(game.empty_space);
    game.turns_left = 9;
}

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
