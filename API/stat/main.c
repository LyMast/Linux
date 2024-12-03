#include <stdio.h>
#include <sys/stat.h>


int main()
{
	struct stat st;
	const char* path = "./Test/";
	int a = 0;

	// [ Verify exist of Directory ]
	// No - return -1
	// Yes - return 0

	a = stat(path, &st);

	sync();

	printf("Result : %d\n", a);

	return 0;
}
