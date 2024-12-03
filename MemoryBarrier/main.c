#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int g_flag[2] = { 0, };
int g_turn = 0;
int g_count = 0;

#define df_LOOP  500000
#define MB

void* test1(void* arg)
{
	int loop = 0;
	while(1)
	{
		g_flag[0] = 1;
		g_turn = 1;

#ifdef MB
		 __sync_synchronize();
#endif

		/* Lock */
		while(g_flag[1] && g_turn == 1);

		/* Critical Section */
		g_count += 1;
		loop += 1;
		/* Critical Section */

		/* Unlock */
		g_flag[0] = 0;

		if (loop == df_LOOP)
			break;
	}

	return 0;
}

void* test2(void* arg)
{
	int loop = 0;
	while(1)
	{
		g_flag[1] = 1;
		g_turn = 0;

#ifdef MB
		 __sync_synchronize();
#endif

		/* Lock */
		while(g_flag[0] && g_turn == 0);

		/* Critical Section */
		g_count += 1;
		loop += 1;
		/* Critical Section */

		/* Unlock */
		g_flag[1] = 0;

		if(loop == df_LOOP)
			break;
	}

	return 0;
}

int main(int argc, char* argv[])
{

	pthread_t threads[2];
	int ret;

	ret = pthread_create(&threads[0], NULL, test1, NULL);
	ret = pthread_create(&threads[1], NULL, test2, NULL);

	for(int i = 0 ; i < 2 ; ++i)
	{
		pthread_join(threads[i], NULL);
	}

	printf("g_count : %d\n", g_count);

}
