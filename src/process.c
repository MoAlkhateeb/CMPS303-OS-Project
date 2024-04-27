#include "headers.h"
#include <sys/signal.h>

volatile int stopedtime = 0, resumedtime = 0;
volatile int remainingtime, entrancetime, originalremainingtime;

void sigtstp_handler();
void sigcont_handler();

int main(int agrc, char * argv[])
{
    initClk();

    signal(SIGTSTP, sigtstp_handler);
    signal(SIGCONT, sigcont_handler);

    remainingtime = atoi(argv[1]);
    entrancetime = getClk();
    originalremainingtime = remainingtime;

    while (remainingtime > 0)
    {
        remainingtime = ( originalremainingtime - ( getClk() - entrancetime ));
    }

    destroyClk(false);

    return 0;
}

void sigtstp_handler() {
    stopedtime = getClk();
    signal(SIGTSTP, sigtstp_handler);
    raise(SIGSTOP);
}

void sigcont_handler() {
    resumedtime = getClk();
    entrancetime = entrancetime + (resumedtime - stopedtime);
    signal(SIGCONT, sigcont_handler);
}
