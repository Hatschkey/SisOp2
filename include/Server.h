#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <vector>
#include <atomic>
#include <pthread.h>
#include <cmath>

#include "data_types.h"
#include "constants.h"
#include "RW_Monitor.h"
#include "BaseSocket.h"

class Server : protected BaseSocket
{
    // Private attributes
    private:

    typedef void (*command_function)(void);
    static std::map<std::string,command_function> available_commands; // All available administrator commands

    static std::atomic<bool> stop_issued; // Atomic thread for stopping all threads
    int server_socket;  // Socket the server listens at for new incoming connections

    struct sockaddr_in server_address;  // Server socket address
    struct sockaddr_in client_address;  // Client socket address
    static int message_history;    // Amount of old group messages to show clients

    pthread_t command_handler_thread; // Thread for handling server
    static std::map<int, pthread_t> connection_handler_threads; // Threads for handling client connections
    static RW_Monitor threads_monitor;  // Monitor for connection handler threads list

    // Public methods
    public:

    /**
     * Class constructor
     * Initializes server socket
     * @param N Amount of old group messages that will be shown to clients upon joining
     */
    Server(int N);

    /**
     * Class destructor, closes any open sockets
     */
    ~Server();

    /**
     * Listens for incoming connections from clients
     * Upon receiving a connection, spawns a new thread to handle communication
     */
    void listenConnections();

    // Private methods
    private:

    /**
     * Lists all available administrator commands to stdout
     */
    static void listCommands();

    /**
     * Sets up the server socket to begin for listening
     */
    void setupConnection();

    /**
     * Handle server administrator commands
     * This should run in a separate thread to the main server thread
     */
    static void *handleCommands(void* arg);

    /**
     * Handle any incoming connections, spawned by listenConnections
     * Normally, there should be one handleConnection thread per connected client
     */
    static void *handleConnection(void* arg);

    /**
     * Lists all threads currently active and what socket they are
     * assigned to
     */
    static void listThreads();
};

#endif