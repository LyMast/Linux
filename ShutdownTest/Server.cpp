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
    int g_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(g_listenSocket == -1)
    {
        printf("[Create Listen Socket Error] errno : %d, %s\n", errno, strerror(errno));
        return -1;
    }

    //*
    struct linger LingerOptval;
    LingerOptval.l_onoff = 1;
    LingerOptval.l_linger = 0;
    int LingerRetval = setsockopt(g_listenSocket, SOL_SOCKET, SO_LINGER, &LingerOptval, sizeof(LingerOptval));
    if(LingerRetval == -1)
    {
        printf("[Linger Option Error] errno : %d, %s\n", errno, strerror(errno));
        return -1;
    }
    std::cout << "Linger Output : " << LingerRetval << std::endl;
    //*/

    // bind
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
   
    serveraddr.sin_port = htons(10000);
    int BindRetval = bind(g_listenSocket, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if(BindRetval)
    {
        printf("[Bind Listen Socket Error] errno : %d, %s\n", errno, strerror(errno));
        return -1;
    }

    






    struct linger ling;
    socklen_t optlen = sizeof(ling);

   


    // listen
    int ListenRetval = listen(g_listenSocket, SOMAXCONN);
    if(ListenRetval == -1)
    {
        printf("[Listen Function Error] errno : %d, %s\n", errno, strerror(errno));
        return -1;
    }



    int clientSocket;
    struct sockaddr_in clientaddr;

    //struct epoll_event ev;
    uint addrlen = sizeof(clientaddr);

    clientSocket = accept(g_listenSocket, (struct sockaddr*)&clientaddr, &addrlen);



    /*
    struct linger LingerOptval;
    LingerOptval.l_onoff = 1;
    LingerOptval.l_linger = 0;
    int LingerRetval = setsockopt(clientSocket, SOL_SOCKET, SO_LINGER, &LingerOptval, sizeof(LingerOptval));
    if(LingerRetval == -1)
    {
        printf("[Linger Option Error] errno : %d, %s\n", errno, strerror(errno));
    }
    //*/

    char IP[30];
    inet_ntop(AF_INET, &clientaddr.sin_addr, IP, sizeof(IP));
    unsigned short Port = ntohs(clientaddr.sin_port);
    printf("Accept Clinet : IP [%s] Port [%u], addrlen : %d\n", IP, Port, addrlen);




    int result = getsockopt(clientSocket, SOL_SOCKET, SO_LINGER, &ling, &optlen);
    if (result == -1) {
        perror("getsockopt");
        close(g_listenSocket);
        return -1;
    }

    if (ling.l_onoff) {
        printf("Linger 옵션이 활성화되었습니다. 기다리는 시간: %d 초\n", ling.l_linger);
    } else {
        printf("Linger 옵션이 비활성화되었습니다.\n");
    }


    shutdown(clientSocket, SHUT_RDWR);
 
    std::cout << "Input to End : ";
    std::string a;
    std:: cin >> a;



    close(clientSocket);


    close(g_listenSocket);





    return 0 ;
}