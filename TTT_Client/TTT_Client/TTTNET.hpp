/*
    Header files for Tic-Tac-Toe (unix) networking
    Author: Na'zeer Greene
    Class: CS341 Computer Networks
*/

#ifndef TTTNET_HPP
#define TTTNET_HPP
    
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>

    #define ISVALIDSOCKET(s) ((s) >= 0)
    #define CLOSESOCKET(s) close(s)
    #define SOCKET int
    #define GETSOCKETERRNO() (errno)

    /*
        The purpose of the union is to make it easier when passing the
        payload into different functions that require differnce access.
        For example, one function might expect a char array to read, while
        another function expects a unsigned char to operate on.
     */
    typedef struct {
        u_int8_t type;
        u_int8_t length; /* max of 256 characters */
        union {
            u_int8_t data; /* primarily for sending moves */
            char msg[256];
        } payload;
    } TYPE_LENGTH_DATA;
    
    /*
        So that either the client or server knows how to interpret any message sent,
        these are the only valid types. These should be the exact same for the client
        and the server.
    */
    enum SERVER_TO_CLIENT_TYPES { REQUESTING_MOVE = 1, SENDING_MSG, SENDING_BOARD, INIT, SIZE };
    enum CLIENT_TO_SERVER_TYPES { SENDING_MOVE = 1, TERMINATED = 200 };

#endif // TTTNET_HPP
