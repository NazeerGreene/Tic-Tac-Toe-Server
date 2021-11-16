#include "TTT.hpp"
#include <random>



int main(int argc, char * argv[]) 
{
//---------------------------- INITIALIZING THE GAME ---------------------------
    GAMESTATE game{
        .player_char = "",
        .opp_char = "",
        .empty_space = "",
        .turns_left = 9
    };

    // initialize the board decorators with default characters
    TTT_default_board_decorators(game.player_char, game.opp_char, game.empty_space);

    // fill the board with initial 
    game.board.fill(game.empty_space);
    game.is_player_turn = true;
    
    //next 2 lines from https://zhang-xiao-mu.blog/2019/11/02/random-engine-in-modern-c/
    //seed + engine necessary to produce pseudo-random numbers for robot player
    unsigned int seed = std::chrono::steady_clock::now().time_since_epoch().count();
    std::default_random_engine engine(seed);

    //store winner character 
    //[options: player char, opponent char, none ("")]
    std::string winner{""};
    std::string out{""};
//------------------------ FINISHED INITIALIZING THE GAME --------------------------
//------------------------------ PLAYING THE GAME -----------------------------------
    out = "\nNew Game!\n";
    std::cout << "\n" << out << "\n";
    //send_to_server(out);


    while (game.turns_left > 0) 
    {
        //------------------player-----------------
        if(game.is_player_turn) 
        {
            std::cout << valid_spaces << std::endl;
            print_board(game);
            std::cout << std::endl;

            unsigned move = valid_input();
            if (move == EOF) exit(0);
            player_move(move, game);

        //----------------not player---------------
        } else {
            //generates "move" in [0..8] 
            auto move = engine() % 9;
            std::cout << "Opponent move: " << move << std::endl;
            //opponent attempts to occupy spot
            robot_move(move, game);
            print_board(game);
        }

        winner = check_for_win(game);
        std::cout << winner << std::endl;
        //no winner
        if (winner == "") 
        {
            continue;
        }
        //if there is a winner...
        out = "\n\n~~~Player " + 
        (winner == game.player_char ? game.player_char : game.opp_char) + 
        " won!~~~\n";

        std::cout << out;
        //send_to_server(out);

        print_board(game);
        clear_board(game);
        break;
    } //end of if(game.is_player_turn) 

    if (game.turns_left == 0) //if turns reach 0, then no one wins
    {
        out = "It's a draw!\n";
        std::cout << out;    //to user
        //send_to_server(out); //to server
        clear_board(game);   //restart game
    }
//--------------------------- FINISHED GAME --------------------------------

    return 0;
}