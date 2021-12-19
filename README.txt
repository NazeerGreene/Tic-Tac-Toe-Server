g++ --std=c++11 TTT.cpp ttt_server.cpp -o server
g++ --std=c++11 ttt_client.cpp -o client
Server Usage: server.out 
Client Usage: client.out [optional: IPv4 addr] [optional: port] (only default port works)

Notes:
1.  There is code present that isn't used. It is for future updates and new features.
2.  There will be noticable glitches if client one disconnects before client two connects (during the initial connection), it is the only major glitch when connectioning clients.
3.  It is not technically necessary for the client to send a formal termanitation to the server, that is there for any future plans.