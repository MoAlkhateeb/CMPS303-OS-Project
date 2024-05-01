#include <time.h>

#include "headers.h"

int main(int agrc, char* argv[]) {
    int remainingTime = atoi(argv[1]);

    /*
        Using clock() instead of getClk() to avoid the off by 1 bugs in logs.

        refering:
       https://www.tutorialspoint.com/c_standard_library/c_function_clock.htm
    */

    while (clock() / (double)CLOCKS_PER_SEC < remainingTime) {
        // Busy Wait Consume CPU
    }
}
