

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

using namespace std;

//#define df_TEST_BLOCK

static int g_listenSocket = -1;
static bool g_ShutDown = false;
static int g_EpollFd;


struct SOCKET_INFO
{
    int Socket;

    ushort Port;
    char IP[INET_ADDRSTRLEN];

    char RecvBuffer[1024];
};

static vector<SOCKET_INFO*> g_list;
static mutex g_listLock;

int main(int argc, char* argv[])
{


    g_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(g_listenSocket == -1)
    {
        printf("[Create Listen Socket Error] errno : %d, %s\n", errno, strerror(errno));
        return -1;
    }


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


    // linger Socket opt
    //*
    struct linger LingerOptval;
    LingerOptval.l_onoff = 1;
    LingerOptval.l_linger = 0;
    int LingerRetval = setsockopt(g_listenSocket, SOL_SOCKET, SO_LINGER, &LingerOptval, sizeof(LingerOptval));
    if(LingerRetval == -1)
    {
        printf("[Linger Option Error] errno : %d, %s\n", errno, strerror(errno));
        return false;
    }
    //*/


    // 소켓 송신버퍼 사이즈 설정



    // Create Epoll
    g_EpollFd = epoll_create1(EPOLL_CLOEXEC);
    if(g_EpollFd < 0)
    {
        printf("Fail to Create Epoll\n");
        return -1;
    }

    // listen
    int ListenRetval = listen(g_listenSocket, SOMAXCONN);
    if(ListenRetval == -1)
    {
        printf("[Listen Function Error] errno : %d, %s\n", errno, strerror(errno));
        return false;
    }





    int clientSocket;
    struct sockaddr_in clientaddr;

    struct epoll_event ev;
    uint addrlen = sizeof(clientaddr);


    memset(&clientaddr, 0, sizeof(clientaddr));
    clientSocket = accept(g_listenSocket, (struct sockaddr*)&clientaddr, &addrlen);
    if(clientSocket == -1)
    {
        printf("AcceptThread Shutdown : %d \n", errno);
        return -1;
    }


    int flags = fcntl(clientSocket, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(clientSocket, F_SETFL, flags);

    SOCKET_INFO info;
    SOCKET_INFO* pInfo = &info;

    pInfo->Socket = clientSocket;
    inet_ntop(AF_INET, &clientaddr.sin_addr, pInfo->IP, sizeof(pInfo->IP));
    pInfo->Port = ntohs(clientaddr.sin_port);







    ////////////////////////////////////////////////////////////////////////////////
    //******* Epoll에 집어 넣기 전에 ShutDown하고서 집어 넣기. *************
    //  
    //      1. ShutDown하고나서 Epoll에 넣으면 잘 넣어진다. + Epoll에 Ret = 0의 결과가 반복된다.
    //      2. 위의 상태에서 Epoll에 삭제 안하면 SOcket 관련된 메시지는 계속 들어오게 된다.
    ////////////////////////////////////////////////////////////////////////////////

    //shutdown(clientSocket, SHUT_RD);









    ev.data.ptr = (void*)pInfo;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;   // 엣지 트리거로 Read, Send 가능 상태를 받는다.
    if(epoll_ctl(g_EpollFd, EPOLL_CTL_ADD, clientSocket, &ev) == -1)
    {
        printf("Fail to Put in Epoll Q\n");  
                        
        close(clientSocket);
        return -1;
    }

    printf("Success to Put in Epoll Q!\n");


    // RecvProc
    while(true)
    {
        struct epoll_event events_list[FD_SETSIZE];
        int iEpollRet;
        iEpollRet = epoll_wait(g_EpollFd, events_list, FD_SETSIZE, -1);

        if(iEpollRet > 0)
        {
            for(int i = 0; i < iEpollRet ; ++i )
            {
                if(events_list[i].events & EPOLLIN)
                {
                    cout << "Recv Event!\n";
                    char* writepos = pInfo->RecvBuffer;
                    memset(pInfo->RecvBuffer, 0 , sizeof(pInfo->RecvBuffer));

                    int RecvRet = recv(pInfo->Socket, writepos, 1024, 0);      

;
                    int Len = 0;

                    while(1)
                    {
                        if(RecvRet == -1)
                        {
                            if(errno == EWOULDBLOCK)
                            {
                                cout << "RECV WOULDBLOCK\n";
                                break;
                            }
                                    
                            printf("Recv Error!!!! : %d , %s\n\n", errno, strerror(errno));




                            //epoll_ctl(g_EpollFd, EPOLL_CTL_DEL, pInfo->Socket, NULL);     
                            shutdown(pInfo->Socket, SHUT_RD);

                            break;
                        }
                        else if(RecvRet == 0)
                        {
                            cout << "REt = 0!! ShutDown !!\n";

                            /////////////////////////////////////////////////
                            //      epoll_ctl을 하고나서 다시 이벤트가 오는지 확인 + ShutDown시 Epoll에 메시지가 오는 상황 확인!
                            //
                            //      >> 아래 epoll_ctl에 위치에 따라서도 차이가 있나 확인했지만 없다.
                            /////////////////////////////////////////////////

                            epoll_ctl(g_EpollFd, EPOLL_CTL_DEL, pInfo->Socket, NULL);     
                            shutdown(pInfo->Socket, SHUT_RD);
                            // epoll_ctl(g_EpollFd, EPOLL_CTL_DEL, pInfo->Socket, NULL);     

                            string aaaa;
                            cout << "\n>>>>>";
                            cin >> aaaa;

                            
                            break;
                        }
                        else
                        {
                            writepos += RecvRet;
                            Len += RecvRet;
                        }
                    }

                }
                if (events_list[i].events & EPOLLOUT)
                {
                    cout << "Send Event!\n";

                }


            }


        }


        cout << "End Cycle -------------------------------------\n";


    }












    

    close(g_listenSocket);

    cout << "End\n";


    close(g_EpollFd);



    


}