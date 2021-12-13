g++ --std=c++11 TTT.cpp ttt_server.cpp -o server
g++ --std=c++11 ttt_client.cpp -o client
Server Usage: server.out 
Client Usage: client.out [optional: IPv4 addr] [optional: port] (only default port works)

Notes:
There is code in the files that I haven't used because (should time allow it) I want to build upon the features of the program. Also, for some reason the select in server.cpp is weird when it comes to stdin, I'm trying to fix that. My client handles erroneous input before it sends to the server, but if the server receives the wrong input then it won't advance a turn until the input is correct (though there might be edge cases). 

Ctrl-D on the client to close the connection. It might not show it on the server, but once the server loses a connection it will search for another one. I'm fixing that, too. 

Also also, on the server output, it might appear to glitch on the robot's turn, however that is merely the robot picking multiple spots until one is valid. The output will state which spot is valid.
