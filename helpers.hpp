//header file for windows, unix compatibility
//includes are os depedent

#ifndef HELPERS_HPP
#define HELPERS_HPP
    
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
    
#endif // HELPERS_HPP