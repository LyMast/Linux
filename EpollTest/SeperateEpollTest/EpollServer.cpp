

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
static int g_SendEventFd;


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
    g_SendEventFd = epoll_create1(EPOLL_CLOEXEC);
    if(g_SendEventFd < 0)
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




    thread AcceptThread = thread([]()->void {

        int clientSocket;
        struct sockaddr_in clientaddr;

        struct epoll_event ev;

        uint addrlen = sizeof(clientaddr);

        while(!g_ShutDown)
        {
            memset(&clientaddr, 0, sizeof(clientaddr));
            clientSocket = accept(g_listenSocket, (struct sockaddr*)&clientaddr, &addrlen);
            if(clientSocket == -1)
            {
                printf("AcceptThread Shutdown : %d \n", errno);
                break;
            }

            int flags = fcntl(clientSocket, F_GETFL);
            flags |= O_NONBLOCK;
            fcntl(clientSocket, F_SETFL, flags);

            SOCKET_INFO* pInfo = new SOCKET_INFO;

            g_listLock.lock();
            g_list.emplace_back(pInfo);
            g_listLock.unlock();

            pInfo->Socket = clientSocket;
            inet_ntop(AF_INET, &clientaddr.sin_addr, pInfo->IP, sizeof(pInfo->IP));
            pInfo->Port = ntohs(clientaddr.sin_port);


            printf("Accept Clinet : IP [%s] Port [%u], addrlen : %d\n", pInfo->IP, pInfo->Port, addrlen);

            memset(&ev, 0, sizeof(epoll_event));
            ev.data.ptr = (void*)pInfo;
            ev.events = EPOLLIN  | EPOLLET;   // 엣지 트리거로 Read, Send 가능 상태를 받는다.
            if(epoll_ctl(g_EpollFd, EPOLL_CTL_ADD, clientSocket, &ev) == -1)
            {
                printf("Fail to Put in Epoll Q\n");

                
                g_listLock.lock();
                g_list.erase(remove(g_list.begin(), g_list.end(), pInfo), g_list.end());
                g_listLock.unlock();
                
                delete pInfo;
                close(clientSocket);
                continue;
            }


            memset(&ev, 0, sizeof(epoll_event));
            ev.data.ptr = (void*)pInfo;
            ev.events = EPOLLOUT  | EPOLLET;   // 엣지 트리거로 Read, Send 가능 상태를 받는다.
            if(epoll_ctl(g_SendEventFd, EPOLL_CTL_ADD, clientSocket, &ev) == -1)
            {
                printf("Fail to Put in Epoll Q\n");

                
                g_listLock.lock();
                g_list.erase(remove(g_list.begin(), g_list.end(), pInfo), g_list.end());
                g_listLock.unlock();
                
                delete pInfo;
                close(clientSocket);
                continue;
            }





            printf("Success to Put in Epoll Q!\n");


        }

    });

    thread EpollThread = thread([]()->void{

        struct epoll_event events_list[FD_SETSIZE];
        int iEpollRet;

        char MessageBuf[2024];
        SOCKET_INFO* pInfo;
        
        while(!g_ShutDown)
        {
            iEpollRet = epoll_wait(g_EpollFd, events_list, FD_SETSIZE, -1);
            if(iEpollRet > 0)
            {
                for(int i = 0; i < iEpollRet ; ++i )
                {
                    pInfo = (SOCKET_INFO*)events_list[i].data.ptr;
                    
                    // RecvProc
                    if(events_list[i].events & EPOLLIN)
                    {
                        printf("[Epoll Recv Edge Event Occur!] : Socket = %d, IP = %s, Port = %d \n", pInfo->Socket, pInfo->IP, pInfo->Port);

                        char* writepos = pInfo->RecvBuffer;
                        memset(pInfo->RecvBuffer, 0 , sizeof(pInfo->RecvBuffer));
                        bool RecvSuccess = false;
                        int Len = 0;

                        while(1)
                        {
                            int RecvRet = recv(pInfo->Socket, writepos, 1024, 0);                            
                            if(RecvRet == -1)
                            {
                                if(errno == EWOULDBLOCK)
                                {
                                    RecvSuccess = true;    
                                    break;
                                }
                                
                                printf("Recv Error!!!! : %d , %s\n\n", errno, strerror(errno));

                                close(pInfo->Socket);
                                delete pInfo;
                                break;
                            }
                            else if(RecvRet == 0)
                            {
                                printf("Recv End  Socket = %d, IP = %s, Port = %d \n", pInfo->Socket, pInfo->IP, pInfo->Port);

                                close(pInfo->Socket);
                                delete pInfo;
                                break;
                            }
                            else
                            {
                                writepos += RecvRet;
                                Len += RecvRet;
                            }
                        }

#ifndef df_TEST_BLOCK
                        if(RecvSuccess) 
                        {
                            printf("[Recv] : %s\n", pInfo->RecvBuffer);


                            //*
                            char* ReadPos = pInfo->RecvBuffer;
                            bool bSend = false;
                            while(Len)
                            {
                                int SendRetval = send(pInfo->Socket, ReadPos, Len, 0);
                                printf("[SendRetval = %d] ", SendRetval);
                                if(SendRetval == -1)
                                {
                                    if(errno == EWOULDBLOCK)
                                    {
                                        printf("Send WOULDBLOCK!! - ");
                                        bSend = true;
                                        break;
                                    }

                                    printf("Send Error!!!! : %d , %s\n\n", errno, strerror(errno));

                                    close(pInfo->Socket);
                                    delete pInfo;

                                    break;
                                }
                                else
                                {
                                    ReadPos += SendRetval;
                                    Len -= SendRetval;
                                    if(!Len)    bSend = true;
                                }

                            }

                            if(bSend) printf("[Send] : %s\n\n", pInfo->RecvBuffer);
                            //*/
                            
                        }
#else
                        if(RecvSuccess)
                        {
                            while(1)
                            {
                                char SendInf[10] = "abcdefghi";
                                int InfRet = send(pInfo->Socket, SendInf, 10, 0);
                                if(InfRet == -1)
                                {
                                    if(errno == EWOULDBLOCK)
                                    {
                                        printf("Can't Send Anymore!!!!!\n\n");
                                        break;
                                    }
                                    
                                    printf("SendInf Error : %d, %s", errno, strerror(errno));
                                    break;
                                }
                            }
                        }          
#endif
                    }

                    // EpollError
                    if(events_list[i].events & EPOLLERR)
                    {

                        printf("[Epoll Error Edge Event Occur!]  \n");
                    }


                    // HangUp?
                    if(events_list[i].events & EPOLLHUP)
                    {

                        printf("[Epoll EPOLLHUP Edge Event Occur!]  \n");
                    }

                    // ShutDown
                    if(events_list[i].events & EPOLLRDHUP)
                    {

                         printf("[Epoll EPOLLRDHUP Edge Event Occur!]  \n");
                    }

                    printf("-------------------------%d-------------------------\n", i);
                }   
            }
            else
            {
                int Error = errno;
                printf("Epoll Error : %d, %d, %s", iEpollRet, Error, strerror(Error));
            }
        }


    });


    thread SendEventThread = thread([]()->void{

        struct epoll_event events_list[FD_SETSIZE];
        int iEpollRet;

        char MessageBuf[2024];
        SOCKET_INFO* pInfo;
        
        while(!g_ShutDown)
        {
            iEpollRet = epoll_wait(g_EpollFd, events_list, FD_SETSIZE, -1);
            if(iEpollRet > 0)
            {
                for(int i = 0; i < iEpollRet ; ++i )
                {
                    if(events_list[i].events & EPOLLOUT)
                    {

                        printf("[S-E][Epoll Send Edge Event Occur!]  \n\n");
                        if(events_list[i].data.ptr == 0)
                        {
                            printf("SendEvent Thread ShutDown!!\n");
                            continue;
                        }
                    }
                    // EpollError
                    if(events_list[i].events & EPOLLERR)
                    {

                        printf("[S-E][Epoll Error Edge Event Occur!]  \n");
                    }


                    // HangUp?
                    if(events_list[i].events & EPOLLHUP)
                    {

                        printf("[S-E][Epoll EPOLLHUP Edge Event Occur!]  \n");
                    }

                    // ShutDown
                    if(events_list[i].events & EPOLLRDHUP)
                    {

                         printf("[S-E][Epoll EPOLLRDHUP Edge Event Occur!]  \n");
                    }

                    printf("-------------------------%d-------------------------\n", i);

                }

            }

        }


    });
















    string str;
    while(!g_ShutDown)
    {
        cout << "Input : ";
        cin >> str;

        if(str == "Q")  
        {
            cout << "Shutdown!" << endl;
            g_ShutDown = true;
        }

        if(str == "delep")
        {
            cout << "Current Socket List Size - " << g_list.size() << " Input Index Delete Socket - ";
            cin >> str;

            int seq = stoi(str);

            SOCKET_INFO* pInfo = g_list[seq];
            
            int socket = pInfo->Socket;

            //close(socket);  // 해당 소켓을 삭제하고나서 del epoll을 해줄 필요가 없다.

            shutdown(socket, SHUT_RD);  // 1번째 관측 : (상황 - shutdown(socket, SHUT_RD)을 호출해주고나서 epoll_ctl Del을 수행.)
                                        //          - 결과 = 단지 ShutDown을 해주고 나서 epoll Q에 대한 변화는 없는 듯하다. -> epoll_ctl Del이 성공한다.

                                        // 2번째 관측 : (상황 - 단지 ShutDown Recv만 하고서 Epoll del을 하지 않은 채 Client의 Recv 받아보기.)
                                        //          - 결과 = epollThread에 에지 트리거 통지가 왔다. flag는 EPOLLIN, EPOLLOUT 모두. 이때, RecvRet은 0으로서 연결이 종료가 되었다.

                                        //  *** 추가적인 해서 *** : TCP는 내 프로그램과 다르게 동작한다. -> 내 프로그래밍 블락되어있다고 해서 TCP가 동작하지 않는 것이다라는 것은 어폐다. -> 해당 완료통지는 어느것에 유래가 되었는지 확인이 필요하다! (와이어 샤크를 통해 확인이 필요. TCP 단위에서 이루어진 것일 수 있기 때문이다.)


                                        // 1, 2의 결과로 파악했을 때, 복합적으로 멀티 스레드 환경에서 그냥 Shutdown 함수만 실행하고 epoll_ctl del을 하지 않는 것이 좋을 듯하다.
                                        //          - 이유 : 현재 main thread에서 shutdown을 하고 -> EpollThread에서 에지 트리거가 발생하며 RecvRet으로 CloseSocket등의 행위를 이어갈 때, 어짜피 1번의 결과 처럼 close(Socket)형태로 epoll에서 제거가 될 것이기 때문이다.
                                        //                  그렇다면 2번 경우라고 하더라도 1번의 경우처럼 epoll_ctl Del이 실패할 것이니까.

                                        // 결론 - 지금 내가 구상한 Disconnect에서 ShutDown 함수만 실행하면 EpollThread에서 IO_COUNT를 내린다면 삭제 시점을 확인할 수 있을 것이다.
                                        //      상대방이 종료 요청한 경우도 마찬가지로 EpollThread에서 RecvRetval = 0을 이용한다. *굳이 epoll_ctl Del을 할 이유가 없다.
            /*
            if(epoll_ctl(g_EpollFd, EPOLL_CTL_DEL, socket, NULL)== -1)
            {
                int error = errno;
                printf("Fail to Delete socket in Epoll Q, %d, %s\n", error, strerror(error));
            }
            //*/
            
        }
    }

    shutdown(g_listenSocket, SHUT_RD);
    

    close(g_listenSocket);

    cout << "End\n";

    AcceptThread.join();
    cout << "Accept Join" << endl; 

    int filedes[2];
    int pipefd = pipe(filedes);
    if(pipefd == -1)
    {
        printf("Fail to create pip!\n");
    }

 

    struct epoll_event ev;
    ev.data.ptr = 0;
    ev.events = EPOLLOUT;   // 엣지 트리거로 Read, Send 가능 상태를 받는다.

    
    if(epoll_ctl(g_EpollFd, EPOLL_CTL_ADD, 0, &ev) == -1)
    {
        printf("Fail to Put in Epoll Q\n");
    }

    if(epoll_ctl(g_SendEventFd, EPOLL_CTL_ADD, 0, &ev) == -1)
    {
        printf("Fail to Put in Epoll Q\n");
    }




    EpollThread.join();
    SendEventThread.join();

    close(filedes[0]);
    close(filedes[1]);
    close(g_EpollFd);
    close(g_SendEventFd);

    cout << "Epoll join" << endl;





}