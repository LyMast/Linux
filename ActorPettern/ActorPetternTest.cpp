
#include <iostream>
#include <thread>
#include <boost/lockfree/queue.hpp>

#include <pthread.h>
#include <semaphore.h>
#include <atomic>


//#define df_CASE1

typedef struct {
    sem_t semaphore;
    std::atomic_bool Check;
} AutoResetEvent;

void AutoResetEvent_Init(AutoResetEvent* event, int initial) {
    sem_init(&event->semaphore, 0, initial);
    event->Check = 0;
}

void AutoResetEvent_WaitOne(AutoResetEvent* event) {
    sem_wait(&event->semaphore);
    event->Check = 0;
}

void AutoResetEvent_Set(AutoResetEvent* event) {
    if(event->Check.exchange(1) == 0)
        sem_post(&event->semaphore);
}

void AutoResetEvent_Reset(AutoResetEvent* event) {
    while (sem_trywait(&event->semaphore) == 0);
}


struct st_JOB
{
    int a;
};

boost::lockfree::queue<st_JOB> Q{1000000};
AutoResetEvent  Event;
bool ShutDown = false;
int DqFail = 0;

int RetCount = 0;

using namespace std;


int main()
{
    //Q.reserve(10000);
    AutoResetEvent_Init(&Event, 0);


    thread ProducerThread([]()->void{

        st_JOB Dq;
        bool CheckFlag = false;

        while(!ShutDown)
        {
            #ifdef df_CASE1
            AutoResetEvent_WaitOne(&Event);

            if(!Q.pop(Dq))
            {
                DqFail++;   // 이 값으로 1이 나온다.    -> 즉, 마지막 1번 ShutDown 할 때 빼고는 나온 Case가 없다는 말이다.
                            // 하지만, 락프리 Q의 원리를 알면은 이 방법이 문제가 존재한다는 것을 알 수 있다.
                continue;
            } 

           

            #else
            
            if(!Q.pop(Dq))
            {
                AutoResetEvent_WaitOne(&Event);
                DqFail++;   // Post한 횟수만큼 나온다.(600001)  // 당연한 얘기.
                CheckFlag = false;
                continue;
            } 

            if(!CheckFlag)
            {
                CheckFlag = true;
                //DqFail++;       // 와... 여기서 1이 나옴    
                                // 해석이면 처음 시작 단 1번만 상승이란 뜻. Wait_One 1번 깨어난것으로 모든 작업을 했다는 의미이다.-> 무의미하게 Continue가 599999번 일어났다는 의미이다.
            }


            #endif

            // cout << Dq.a << endl;
            RetCount++;

        }

        return;
    });

    thread ConsumerThread([]()->void {

        for(int i = 0 ; i < 100000 ; i ++)
        {
            st_JOB input;
            input.a = i;
            
            Q.push(input);

            AutoResetEvent_Set(&Event);
        }

        
        cout << "Clear" << endl;
        int a;
        cin >> a;

        ShutDown = true;
        AutoResetEvent_Set(&Event);

        return;
    });


    thread Arr[10];

    for(auto& Element : Arr)
    {
        Element = thread ([]()->void {

        for(int i = 0 ; i < 50000 ; i ++)
        {
            st_JOB input;
            input.a = i;
            
            Q.push(input);

            AutoResetEvent_Set(&Event);
        }

        return;

        });
    }
    



    ProducerThread.join();
    ConsumerThread.join();
    for(auto& Element : Arr)
    {
        Element.join();
    }



    if(!Q.empty())
        cout << "Not Empty!!!" << endl;

    if(DqFail)
        cout << "Exist Exceptiom!! - " << DqFail << endl;

     cout << "Count = " << RetCount <<  "// End!" << endl;

}
