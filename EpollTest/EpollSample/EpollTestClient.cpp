

#include <iostream>
#include <thread>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

//#define df_TEST_BLOCK


static bool g_ShutDown = false;


int main(int argc, char* argv[])
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

    // Connect 

    int Retval = connect(Socket, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if(Retval == -1)
    {
        cout << "connect Error\n";
        return -1;
    }

 
    int Len = 0;
    string Input;

    while(!g_ShutDown)
    {  
        Len = 0;

        cout << "Input : ";
        cin >> Input;
        if(Input == "Q")
        {
            cout << "ShutDown!!\n\n";
            break;
        }

        cout << "[Send] : " << Input <<endl;

        Retval = send(Socket, Input.c_str(), Input.size(), 0);
        if(Retval == -1)
        {
            cout << "Fail to Send\n\n";
            break;
        }


#ifndef df_TEST_BLOCK
        //*     에코 동작의 코드.
        Len += Retval;
        char Recv[1024];
        memset(Recv, 0 , sizeof(Recv));
        char* pWrite = Recv;
        

        while(Len)
        {
            Retval = recv(Socket, pWrite, 1024, 0);
            if(Retval == -1)
            {
                cout << "Fail to Recv\n\n";
                break;
            }
            if(Retval == 0)
            {
                g_ShutDown = true;
                break;
            }
            Len -= Retval;
            pWrite += Retval;
            printf("[RecvRetval = %d] ", Retval);
        }

        printf("[Recv] : %s\n\n", Recv);
        //*/
        
#else

        // Recv를 하지 않아서 서버의 송신버퍼를 꽉 채우게끔 하는 것이 목적인 코드
        //*
        char BlockRecv[1000000] = {1,};
        string ForBlock;
        while(ForBlock != "Start")
        {
            // Windows 시스템 처럼 Overlapped IO로 동작하는 지 확인해보는 코드.
            for(int i = 0 ; i < 20 ; i++)
                cout << BlockRecv[i] << " ";
            cout << "\n";
            cin >> ForBlock;
        }
        Retval = recv(Socket, BlockRecv, 1000000, 0);
        cout << Retval << endl;        
        //*/

#endif



    }



    close(Socket);
    




}