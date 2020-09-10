#include <regex>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>
#include <unistd.h>
#include <iostream>
#include <atomic>
#include <pthread.h>

#include "constants.h"
#include "data_types.h"

class Client
{
    // Private attributes
    private:

    static std::atomic<bool> stop_issued;   // Atomic flag for stopping all threads
    std::string username;                   // Client display name
    std::string groupname;                  // Group the client wishes to join
    std::string server_ip;                  // IP of the remote server
    int         server_port;                // Port the remote server listens at
    static int  server_socket;              // Socket for remote server communication
    struct sockaddr_in server_address;      // Server socket address

    pthread_t server_listener_thread;       // Thread to listen for new incoming server messages

    // Public methods
    public:
    /**
     * Class constructor
     * @param username User display name, must be between 4-20 characters (inclusive) and be composed of only letters, numbers and dots(.)
     * @param groupname Chat room the user wishes to join, must be between 4-20 characters (inclusive) and be composed of only letters, numbers and dots(.)
     * @param server_ip IP Address for the remote server, formatted in IPv4 (x.x.x.x)
     * @param server_port Port the remote server is listening at
     */
    Client(std::string username, std::string groupname, std::string server_ip, std::string server_port);
 
    /**
     * Class destructor
     * Closes communication socket with the remote server if one has been opened
     */
    ~Client();

    /**
     * Setup connection to remote server
     */
    void setupConnection();

    /**
     * Handles getting user input so that it may be sent to the remtoe server
     */
    void handleUserInput();

    private:

    /**
     * Poll server for any new messages, showing them to the user
     * This should run in a separate thread to handleUserInput
     */
    static void *getMessages(void* arg);

    /**
     * Send packet to server with login information (User and Group)
     * @return Number of bytes sent to remote server
     */
    int sendLoginPacket();
    
    /**
     * Send packet to server with a chat message
     * @param message Message to be sent to the chatroom
     * @return Number of bytes sent to remote server
     */
    int sendMessagePacket(std::string message);

};