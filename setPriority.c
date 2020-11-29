#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {

    if(argc < 3 || argc > 3) {
        printf(2, "Invalid arguments: Usage setPriority [newPriority] [pid]\n");
        exit();
    }
    int newPriority = atoi(argv[1]);
    if(newPriority < 0 || newPriority > 100) {
        printf(2, "The priority of a process has to be between 1 and 100, inclusive. Please try again.\n");
        exit(); 
    }
    int pid = atoi(argv[2]);
    
    printf(1, "%d %d\n", newPriority, pid);

    int old = set_priority(newPriority, pid);
    if(old < 0) 
        printf(2, "No process with the given pid exists.\n");
    
    exit();
}