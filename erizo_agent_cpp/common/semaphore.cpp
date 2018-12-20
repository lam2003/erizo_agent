#include "semaphore.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

DEFINE_LOGGER(Semaphore, "Semaphore");

Semaphore::Semaphore() : init_(false), sem_id_(0) {}
Semaphore::~Semaphore() {}

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *arry;
};

int Semaphore::init(int key)
{
    if (init_)
    {
        ELOG_WARN("Semaphore duplicate initialize,just reture!!!");
        return 0;
    }
    sem_id_ = semget((key_t)key, 1, 0666 | IPC_CREAT);
    if (sem_id_ < 0)
    {
        ELOG_ERROR("Create semaphore failed");
        return 1;
    }

    union semun sem_union;
    sem_union.val = 0;
    if (semctl(sem_id_, 0, SETVAL, sem_union) == -1)
    {
        ELOG_ERROR("Semaphore set value failed");
        return 1;
    }
    init_ = true;
    return 0;
}

void Semaphore::close()
{
    if (!init_)
    {
        ELOG_WARN("Semaphore didn't initialize,can't close!!!");
        return;
    }
    union semun sem_union;
    semctl(sem_id_, 0, IPC_RMID, sem_union);
}

void Semaphore::semaphoreP()
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id_, &sem_b, 1) < 0)
        ELOG_ERROR("SemaphoreP failed");
}

void Semaphore::semaphoreV()
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id_, &sem_b, 1) < 0)
        ELOG_ERROR("SemaphoreV failed");
}