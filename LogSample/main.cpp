
#include <iostream>
#include <cstring>
#include <stdarg.h>


#define df_BUFFER_SIZE      1024

static char g_TempBuffer[df_BUFFER_SIZE] = {0,};
void Log(const char* szType, int LogLevel, const char* szStringFormat, ...);



int main(int argc, char* argv[])
{


    Log("TEST", 1, "print please!!! One Time Test!!\n\n");

    //*
    Log("TEST", 1, "print please int = %d, string = %s, float = %f\n",
                100, "good Job!", 10.123);
    //*/

    



    return 0;
}



void Log(const char* szType, int LogLevel, const char* szStringFormat, ...)
{

    memset(g_TempBuffer, 0, df_BUFFER_SIZE);
    printf("[%s] ", szType);

    va_list ap;
    va_start(ap, szStringFormat);
    auto reslut = vsnprintf(g_TempBuffer, sizeof(g_TempBuffer), szStringFormat, ap);
    va_end(ap);

    printf("%s\n\n", g_TempBuffer);


    return;
}