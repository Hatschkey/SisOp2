#ifndef ELECTIONLISTENER_H
#define ELECTIONLISTENER_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <vector>
#include <atomic>
#include <cmath>
#include "CommunicationUtils.h"

class ElectionListener : protected CommunicationUtils
{
    // Private attributes
    private:
    static std::atomic<bool> stop_issued; // Atomic thread for stopping all threads
    static int server_socket;  // Socket the server listens at for new incoming connections
    struct sockaddr_in client_address;  // Client socket address
    static int listen_port;
    struct sockaddr_in server_address;  // Server socket address

    // Public methods
    public:
    /**
     * Class constructor
     * Initializes server socket
     * @param N Port to listen for connections
     */
    ElectionListener(int N);

    /**
     * Class destructor, closes any open sockets
     */
    ~ElectionListener();

    /**
     * Listens for incoming connections from clients
     */
    void listenConnections();

    // Private methods
    private:
    /**
     * Sets up the server socket to begin for listening
     */
    void setupConnection();

    /**
     * Handle any incoming connections, spawned by listenConnections
     */
    static void handleConnection(int socket);
};

#endif