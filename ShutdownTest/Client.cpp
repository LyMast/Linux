#include <iostream>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>


int main()
{

    int Socket = socket(AF_INET, SOCK_STREAM, 0);
    if(Socket == -1)
    {
        printf("[Create Socket Error] errno : %d, %s\n", errno, strerror(errno));
        return -1;
    }

    char ServerIP[] = "127.0.0.1";
    short Port = 10000;

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, ServerIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(10000);


    struct linger LingerOptval;
    LingerOptval.l_onoff = 1;
    LingerOptval.l_linger = 0;
    int LingerRetval = setsockopt(Socket, SOL_SOCKET, SO_LINGER, &LingerOptval, sizeof(LingerOptval));
    if(LingerRetval == -1)
    {
        printf("[Linger Option Error] errno : %d, %s\n", errno, strerror(errno));
        return -1;
    }


    // Connect 

    int Retval = connect(Socket, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if(Retval == -1)
    {
        std::cout << "connect Error\n";
        return -1;
    }


    std::cout << Input to End : 
    std::string a;
    std:: cin >> a;


    close(Socket);




    return 0;
}