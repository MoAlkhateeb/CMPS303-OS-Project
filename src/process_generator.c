#include "headers.h"
#define SHKEY 300

void clearResources(int);
struct processData
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
};

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // TODO Initialization

    FILE *file = fopen("processes.txt", "r");
    if (!file)
    {
        perror("error in open file");
    }
    char line[100];
    char c;
    int line_count = 0;

    do
    {
        if (line[0] != '#')
            line_count = line_count + 1;
    } while (fgets(line, sizeof(line), file));

    printf("line count %d\n", line_count);
    fseek(file, 0, SEEK_SET);

    struct processData *p = malloc(sizeof(int) * line_count);
    int i = 0;
    while (fgets(line, sizeof(line), file))
    {
        if (line[0] == '#')
        {
            continue;
        }
        int id, arrival, runtime, priority;
        sscanf(line, "%d\t%d\t%d\t%d", &id, &arrival, &runtime, &priority);
        p[i].arrivaltime = arrival;
        p[i].id = id;
        p[i].runningtime = runtime;
        p[i].priority = priority;
        printf("p %d arrival time: %d\n", i, p[i].arrivaltime);
        printf("p %d id: %d\n", i, p[i].id);
        printf("p %d runtime: %d\n", i, p[i].runningtime);
        printf("p %d priority: %d\n", i, p[i].priority);
        printf("\n");
        i++;
    }

    // 1. Read the input files.
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    // initClk();
    // To get time use this
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources

    int pid = fork();
    if (pid == 0)
    {
        char *args[] = {"./clk.out", NULL};
        execv("./clk.out", args);
    }
    sleep(1);
    initClk();
    int x = getClk();
    printf("current time is %d\n", x);
    for (int j = 0; j < i; j++)
    {
        do
        {
            x = getClk();
        } while (p[j].arrivaltime > x);
        printf("p %d arrival time: %d\n", j, p[j].arrivaltime);
        printf("p %d id: %d\n", j, p[j].id);
        printf("p %d runtime: %d\n", j, p[j].runningtime);
        printf("p %d priority: %d\n", j, p[j].priority);
        printf("\n");
    }
    destroyClk(true);
}

void clearResources(int signum)
{
    destroyClk(true);
    exit(0);
}
