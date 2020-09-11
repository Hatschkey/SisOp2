#include "RW_Monitor.h"

void RW_Monitor::requestRead()
{
    // Wait until there are no more writers
    while(this->num_writers > 0) ok_read.wait(lock);

    // Increase reader count
    num_readers++;
}

void RW_Monitor::requestWrite()
{
    // Wait until there are no more readers or writers
    while(this->num_writers > 0 || this->num_readers > 0) ok_write.wait(lock);

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
        //write_lock.unlock();    // Unlock the writer lock
        ok_write.notify_one();  // Signal any writer thread
    }
}

void RW_Monitor::releaseWrite()
{
    // Decrease number of writers
    num_writers--;

    // Signal all reader threads
    //read_lock.unlock();
    ok_read.notify_all();
    
    // Signal any writer thread
    //write_lock.unlock();
    ok_write.notify_one();
}