
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
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
ErizoAgent agent;
void sigchld_handler(int signo)
{
    pid_t child_pid = waitpid(-1, NULL, 0);
    if(child_pid > 0) {
        agent.removeErizo(child_pid);
    }
}

void sigint_handler(int signo) 
{  
    agent.close();
    exit(0);
}

void sigterm_handler(int signo) 
{
    agent.close();
    exit(0);
}

int main()
{
    // signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigterm_handler);
    Config::getInstance()->init("config.json");
    agent.init();
    while (1)
        usleep(0);
    return 0;
}