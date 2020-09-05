#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <chrono>

#include "data_types.h"
#include "constants.h"

class Server
{
    // Private attributes
    private:

    /*
        * Data shared between server threads
        * TODO This should be MUTEX protected, as it is accessed concurrently by multiple threads
        */
    static managed_data_t shared_data;
    int server_socket;  // Socket the server listens at for new incoming connections
    int client_socket;  // Socket the client connects from
    struct sockaddr_in server_address;  // Server socket address
    struct sockaddr_in client_address;  // Client socket address
    int message_history;    // Amount of old group messages to show clients

    pthread_t command_handler_thread; // Thread for handling server
    std::vector<pthread_t> connection_handler_threads; // Threads for handling client connections

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

    // Private methods
    private:

    /**
     * Sets up the server socket to begin for listening
     */
    void setupConnection();

};