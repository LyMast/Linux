#include <stdio.h>
#include <time.h>


void PrintNanoTime(void);

int main() {
    struct timespec ts;
    if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
        printf("Seconds since boot: %ld\n", ts.tv_sec);
        printf("Nanoseconds since boot: %ld\n", ts.tv_nsec);
    } else {
        perror("clock_gettime");
    }


	/*
	for(;;)
		PrintNanoTime();
	//*/

	struct time_t tm;
	time(&tm);


    return 0;
}

void PrintNanoTime(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_BOOTTIME, &ts);
	printf("MS Since boot: %ld\n", ts.tv_nsec / 1000);

	return;
}

