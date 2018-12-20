
#include <unistd.h>

#include "common/config.h"
#include "core/erizo_agent.h"

// static void sigchld_handler(int signo)
// {
//     exit(0);
// }
// static void sigint_handler(int signo)
// {
//     exit(0);
// }
// static void sigterm_handler(int signo)
// {
//     exit(0);
// }

int main()
{
    // signal(SIGCHLD, sigchld_handler);
    // signal(SIGINT, sigint_handler);
    // signal(SIGTERM, sigterm_handler);
    Config::getInstance()->init("config.json");
    ErizoAgent agent;
    agent.init();
    while (1)
        usleep(0);
    return 0;
}