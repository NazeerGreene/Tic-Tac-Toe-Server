#include <iostream>
#include <chrono>
#include <random>
#include "TTT.cpp"

int main(int argc, char * argv[]) 
{
    //next 2 lines from https://zhang-xiao-mu.blog/2019/11/02/random-engine-in-modern-c/
    //seed + engine necessary to produce pseudo-random numbers
    unsigned int seed = std::chrono::steady_clock::now().time_since_epoch().count();
    std::default_random_engine e(seed);
    
    TTT session("X", "O", "~");

    //a remote connection will only be initiated if the
    //user provides an address
    if (argc > 1)
        session.establish_connection(argv[1]);

    //.game() must be provided a generator for opponent
    session.game(e);

    std::cout << "Your points: " << session.get_points() << std::endl;

    session.game(e);

    std::cout << "Your points: " << session.get_points() << std::endl;

    return 0;
}