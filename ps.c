#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {

    if(argc < 1 || argc > 1) {
        printf(2, "Invalid arguments: Usage ps\n");
        exit();
    }
    getPinfo();
    exit();
}