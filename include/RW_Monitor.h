#ifndef RW_MONITOR_H
#define RW_MONITOR_H

#include <condition_variable>
#include <mutex>
#include <iostream>

class RW_Monitor
{
    public:

    int num_readers, num_writers;                               // Current number of readers and writers
    std::condition_variable ok_read, ok_write;                  // Condition variables for readers and writers

    std::unique_lock<std::mutex> lock;
        
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