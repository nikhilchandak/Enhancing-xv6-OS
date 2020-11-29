#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

#define vll volatile long long int
#define MAX 500000 // 5e5 -- low time since lot of printing (IO) time is required 

int main(int argc, char *argv[])
{	
	#ifdef MLFQ
		printf(1, "MLFQ running...\n");
	#endif 

	for (int i = 0; i < 7; i++)
	{
		int pid = fork();

		if (pid < 0)
		{
			printf(2, "Failed to fork\n");
			exit();
		}

		else if (pid == 0)
		{
			for(int j=0; j < 7; j++)
			{
				if(j <= i)
					sleep(200);
				else
				{
					for (vll k = 0; k < MAX; k++)
						k = 1 ^ k; //cpu
				}
				
			}

			exit();
		}
	}
	for (int i = 0; i < 10; i++)
		wait();

    exit();
}
