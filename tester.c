#include "types.h"
#include "stat.h"
#include "user.h"

#define ll long long int 

// Parent forks two children, waits for them to exit and then finally exits
int main(int argc, char* argv[]) {
#ifdef FCFS
    int count = 10, lim = 1e7;
    printf(1, "in FCFS\n");
    for (int j = 0; j < count; j++) {
        if (fork() == 0) {
            volatile int a = 0;
            for (volatile int i = 0; i <= lim; i++) {
                a += 3;
            }
            // printf(1, "%d\n", a);
            exit();
        }
    }

    for (int i = 0; i < count; i++) {
        wait();
    }
    int pid = getpid();
    int ret = displayStats(pid);
    printf(1, "Return value: %d\n", ret);
    exit();
#endif

#ifdef PBS
    printf(1, "in PBS\n");
    int MAX = 1e7 ; 
    for (int i = 0; i < 10; i++) 
    {
        int pid = fork();
        if (pid == 0) 
        {
            #ifdef PBS
                int my_pid = getpid();
                set_priority(my_pid, i*10 + 1);
            #endif
            int x = 0;
            for (int y = 0; y < MAX; y++) 
                x = (x+y) %147;

            exit();
        }
    }
    int pid = getpid();
    int ret = displayStats(pid);
    printf(1, "Return value: %d\n", ret);
    exit();
#endif

#ifdef MLFQ
    printf(1, "in MLFQ\n");
    printf(1, "use tester2 instead\n");
    exit();
#endif

#ifdef DEFAULT
    printf(1, "Running default: RR\n"); 

    int count = 10, lim = 1e7;
    for (int j = 0; j < count; j++) {
        if (fork() == 0) {
            volatile int a = 0;
            // int pid = getpid();
            for (volatile int i = 0; i <= lim; i++) {
                if (i % (lim / 10) == 0) {
                    // printf(1, "Completed %d by %d of pid %d\n", i / (lim / 10),
                           // 10, pid);
                }
                a += 3;
            }
            // printf(1, "%d\n", a);
            exit();
        }
    }

    for (int i = 0; i < count; i++) {
        wait();
    }
    int pid = getpid();
    int ret = displayStats(pid);
    printf(1, "Return value: %d\n", ret);
    exit();
#endif
}