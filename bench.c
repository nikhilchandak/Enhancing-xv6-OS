#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

#define vll volatile long long int
#define MAX 50000000 // 5e7

int main(int argc, char *argv[])
{	
	#ifdef PLOT
		printf(1, "time to plot\n");
	#endif 

	printf(1, "Before Benchmarking:\n");
	displayStats(getpid());
	printf(1, "\nBenchmarking..\n");

	for (int i = 0; i < 10; i++)
	{
		int pid = fork();

		if (pid < 0)
		{
			printf(2, "Failed to fork\n");
			exit();
		}

		else if (pid == 0)
		{
			#ifdef PBS
				set_priority(getpid(), 100 - 9 * i);
			#endif
			
			for(int j=0; j < 10; j++)
			{
				if(j <= i)
				// if(j % 2 == 1)
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

    displayStats(getpid());
    // printf(1, "Return value: %d\n", ret);

    exit();
}







// #include "types.h"
// #include "user.h"

// int number_of_processes = 10;

// int main(int argc, char *argv[])
// {
//   int j;
//   for (j = 0; j < number_of_processes; j++)
//   {
//     int pid = fork();
//     if (pid < 0)
//     {
//       printf(1, "Fork failed\n");
//       continue;
//     }
//     if (pid == 0)
//     {
//       volatile int i;
//       for (volatile int k = 0; k < 20; k++)
//       {
//           volatile int x = 0;
//           for (i = 0; i < 10000000; i++)
//           {
//             x++; //cpu time
//           }
//       }
//       exit();
//     }
//   }
//   for (j = 0; j < number_of_processes; j++)
//   {
//     wait();
//   }
//   displayStats(getpid()); 
//   exit();
// }
