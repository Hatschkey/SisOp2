#ifndef RW_MONITOR_H
#define RW_MONITOR_H

#include <iostream>
#include <atomic>
#include <pthread.h>

class RW_Monitor
{
    public:
    std::atomic<int> num_readers, num_writers;  // Current number of readers and writers
    pthread_cond_t ok_read, ok_write;           // Condition variables for reading and writing
    pthread_mutex_t lock;                       // Mutex lock for the incoming requests to wait

    //std::unique_lock<std::mutex> lock;


    /**
     * Class constructor
     * Initializes num_readers and num_writers with 0 
     */
    RW_Monitor();

    /**
     * Requests to perform a read operation on the data
     * If another thread is currently writing, blocks caller until writing is done
     * If not, allows read operation to proceed
     */
    void requestRead();

    /**
     * Requests to perform a write operation on the data
     * If any other threa is reading, block caller until reading is done
     * If not, allows write operation to proceed
     */
    void requestWrite();

    /**
     * Releases lock on a read operation
     * Signal a thread waiting on ok_write to start writing
     */
    void releaseRead();

    /**
     * Releases lock on a write operation
     * Signal all reader threads waiting on ok_read to start reading
     * Signal one thread on ok_write to start writing
     */
    void releaseWrite();
};

#endif