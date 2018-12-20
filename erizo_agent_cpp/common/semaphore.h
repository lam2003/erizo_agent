#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <logger.h>

class Semaphore
{
    DECLARE_LOGGER();

  public:
    Semaphore();
    ~Semaphore();

    int init(key_t key);
    void close();
    void semaphoreP();
    void semaphoreV();

  private:
    bool init_;
    int sem_id_;
};

#endif