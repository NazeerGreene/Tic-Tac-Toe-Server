//header files for Tic-Tac-Toe (unix) networking

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

    typedef struct {
    u_int8_t type;
    u_int8_t length; /* max of 255 characters */
    u_int8_t payload[255];
    } TYPE_LENGTH_DATA;

    enum SERVER_TO_CLIENT_TYPES { REQUESTING_TAG = 1, REQUESTING_MOVE, SENDING_MSG };
    enum CLIENT_TO_SERVER_TYPES { SENDING_TAG = 1, SENDING_MOVE, ACK, TERMINATED = 200 };
    
#endif // TTTNET_HPP
