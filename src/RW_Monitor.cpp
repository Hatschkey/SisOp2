#include "RW_Monitor.h"


RW_Monitor::RW_Monitor()
{
    // Initialize reader and writer variables with 0
    num_readers = 0;
    num_writers = 0;

    // Initialize condition variables and mutex
    pthread_cond_init( &ok_read, NULL);
    pthread_cond_init( &ok_write, NULL);
    pthread_mutex_init( &lock, NULL);
}

void RW_Monitor::requestRead()
{
    // Wait until there are no more writers
    while(this->num_writers > 0) pthread_cond_wait(&ok_read, &lock);

    // Increase reader count
    num_readers++;
}

void RW_Monitor::requestWrite()
{
    // Wait until there are no more readers or writers
    while(this->num_writers > 0 || this->num_readers > 0) pthread_cond_wait(&ok_write, &lock);

    // Increase writer count
    num_writers++;
}

void RW_Monitor::releaseRead()
{
    // Decrease number of readers
    num_readers--;

    // If no readers are left, notify the writer thread
    if (num_readers == 0)
    {
        pthread_cond_signal(&ok_write);
    }
}

void RW_Monitor::releaseWrite()
{
    // Decrease number of writers
    num_writers--;

    // Signal all reader threads
    pthread_cond_broadcast(&ok_read);

    // Signal any writer thread
    pthread_cond_signal(&ok_write);
}