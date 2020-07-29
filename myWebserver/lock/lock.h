#ifndef LOCK_H_
#define LOCK_H_

#include<exception>
#include<pthread.h>
#include<semaphore.h>


class sem
{
public:
    sem()
    {
        sem_init(&m_sem, 0, 0);
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }

    bool wait() //v操作，-1
    {
        return (sem_wait(&m_sem) == 0);
    }
    bool post() //p操作，+1
    {
        return (sem_wait(&m_sem) == 0);
    }

private:
    sem_t m_sem;
};

class locker
{
public:
    locker()
    {
        pthread_mutex_init(&m_mutex, 0);
    }
    ~locker ()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    pthread_mutex_t *get()
    {
        return &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;
};


class cond
{
public:
    cond()
    {
        pthread_cond_init(&m_cond, NULL);
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }

    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, m_mutex);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }

    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }

    bool broadcast() //pthread_cond_timedwait
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }
    
private:
    pthread_cond_t m_cond;
};
#endif