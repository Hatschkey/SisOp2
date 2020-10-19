#include "Server.h"
#include "User.h"
#include "Group.h"

std::atomic<bool> Server::stop_issued;
typedef void (*command_function)(void);
std::map<std::string, command_function> Server::available_commands;
int Server::message_history;
int Server::server_socket;

std::map<int, pthread_t> Server::connection_handler_threads;
RW_Monitor Server::threads_monitor;

Server::Server(int N)
{
    if (N <= 0)
        throw std::runtime_error("Invalid N, must be > 0");

    this->message_history = N;

    // Initialize shared data
    stop_issued = 0;

    // Initialize available server administrator commands
    available_commands.insert(std::make_pair("list users", &User::listUsers));
    available_commands.insert(std::make_pair("list groups", &Group::listGroups));
    available_commands.insert(std::make_pair("list threads", &Server::listThreads));
    available_commands.insert(std::make_pair("stop", &Server::issueStop));
    available_commands.insert(std::make_pair("help", &Server::listCommands));

    // Setup socket
    setupConnection();
}

Server::~Server()
{
    // Close opened sockets
    close(server_socket);
}

void Server::listenConnections()
{
    int client_socket = -1; // Socket assigned to each client on accept

    // Create struct for socket timeout and set USER_TIMEOUT seconds
    struct timeval timeout_timer;
    timeout_timer.tv_sec = USER_TIMEOUT;
    timeout_timer.tv_usec = 0;

    // Set passive listen socket
    if (listen(server_socket, 3) < 0)
        throw std::runtime_error(appendErrorMessage("Error setting socket as passive listener"));

    // Spawn thread for listening to administrator commands
    pthread_create(&command_handler_thread, NULL, handleCommands, NULL);

    // Admin instructions
    std::cout << "Server is ready to receive connections" << std::endl;
    Server::listCommands();

    // Wait for incoming connections
    int sockaddr_size = sizeof(struct sockaddr_in);
    int *new_socket;
    while (!stop_issued && (client_socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t *)&sockaddr_size)) > 0)
    {
        // Start new thread for new connection
        pthread_t comm_thread;

        // Get reference to client socket
        new_socket = (int *)malloc(sizeof(int));
        *new_socket = client_socket;

        // Set the keep-alive timer on the socket
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_timer, sizeof(timeout_timer)) < 0)
            throw std::runtime_error(appendErrorMessage("Error setting socket options"));

        // Spawn new thread for handling that client
        if (pthread_create(&comm_thread, NULL, handleConnection, (void *)new_socket) < 0)
        {
            // Close socket if no thread was created
            std::cerr << "Could not create thread for socket " << client_socket << std::endl;
            close(client_socket);
        }

        // Request write rights
        threads_monitor.requestWrite();

        // Add thread to list of connection handlers
        connection_handler_threads.insert(std::make_pair(*new_socket, comm_thread));

        // Release write rights
        threads_monitor.releaseWrite();
    }

    // Request read rights
    threads_monitor.requestRead();

    // Socket associated with each thread
    int thread_socket = -1;

    // Wait for all threads to finish
    for (std::map<int, pthread_t>::iterator i = connection_handler_threads.begin(); i != connection_handler_threads.end(); ++i)
    {

        std::cout << "Waiting for client communication to end on socket " << i->first << "..." << std::endl;
        // Get the thread reference
        pthread_join(i->second, NULL);

        // Save the thread number
        thread_socket = i->first;

        // Remove this ended thread from the thread list
        connection_handler_threads.erase(thread_socket);
    }

    std::cout << "Waiting for command handler to end..." << std::endl;

    // Join with the command handler thread
    pthread_join(command_handler_thread, NULL);

    // Release read rights
    threads_monitor.releaseRead();
}

void Server::listCommands()
{
    // Iterate map listing commands
    std::cout << "Available commands are: " << std::endl;
    for (std::map<std::string, command_function>::iterator i = Server::available_commands.begin(); i != Server::available_commands.end(); ++i)
    {
        std::cout << " " << i->first << std::endl;
    }
}

void *Server::handleCommands(void *arg)
{
    // Get administrator commands until Ctrl D is pressed or 'stop' is typed
    std::string command;
    while (!stop_issued && std::getline(std::cin, command))
    {
        try
        {
            // Execute administrator command
            available_commands.at(command)();
        }
        catch (std::out_of_range &e)
        {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }

    // Signal all other threads to end
    if (!stop_issued)
        Server::issueStop();

    pthread_exit(NULL);
}

void *Server::handleConnection(void *arg)
{
    int socket = *(int *)arg;        // Client socket
    int read_bytes = -1;             // Number of bytes read from the message
    char client_message[PACKET_MAX]; // Buffer for client message, maximum of PACKET_MAX bytes
    message_record *login_message;   // Buffer for client login information

    message_record *read_message;


    Session *current_session = NULL; // Current session instance for a client

    while ((read_bytes = recv(socket, client_message, PACKET_MAX, 0)) > 0)
    {
        // Decode received data into a packet structure
        packet *received_packet = (packet *)client_message;

        // Decide action according to packet type
        switch (received_packet->type)
        {
        case PAK_DATA: // Data packet

            // Decode received message into a message record
            read_message = (message_record *)received_packet->_payload;

            // Send message
            if (current_session != NULL)
                current_session->messageGroup(read_message);

            break;

        case PAK_COMMAND: // Command packet (login)

            // Get user login information
            login_message = (message_record *)received_packet->_payload;

            // (Try to) Create session
            current_session = new Session(login_message->username, login_message->_message, socket);

            // If session creation went ok
            if (current_session->isOpen())
            {

                // Send history to client
                current_session->sendHistory(Server::message_history);

                /*
                if (!stop_issued && user->getSessionCount(groupname) == 1) // TODO remake this
                {
                    message = "User [" + user->username + "] has joined.";
                    group->post(message, user->username, SERVER_MESSAGE);
                }
                */
            }
            else
            {
                // Reject connection
                close(socket);

                // Delete current session
                delete (current_session);

                // Free received argument pointer
                free(arg);

                // Request write rights
                threads_monitor.requestWrite();

                // Remove itself from the threads list
                connection_handler_threads.erase(socket);

                // Release write rights
                threads_monitor.releaseWrite();

                // Exit
                pthread_exit(NULL);
            }

            break;
        case PAK_KEEP_ALIVE: // Keep-alive packet

            break;
        default:
            std::cout << "Unkown packet received from socket at " << socket << std::endl;
            break;
        }

        // Clear the message buffer
        bzero((void *)client_message, PACKET_MAX);
    }

    // Close current session
    delete (current_session);

    // Check if connection ended due to timeout
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        // If so, close the connection for good
        // OBS: No point in sending message to client, application probably froze
        close(socket);
    }

    // Free received argument
    free(arg);

    if (!stop_issued)
    {
        // Request write rights
        threads_monitor.requestWrite();

        // Remove itself from the threads list
        connection_handler_threads.erase(socket);

        // Release write rights
        threads_monitor.releaseWrite();
    }

    // Exit
    pthread_exit(NULL);
}

void Server::setupConnection()
{
    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error(appendErrorMessage("Error during socket creation"));

    // Prepare server socket address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);

    // Set socket options
    int yes = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        throw std::runtime_error(appendErrorMessage("Error setting socket options"));

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        throw std::runtime_error(appendErrorMessage("Error during socket bind"));
}

void Server::listThreads()
{
    // Request read rights
    threads_monitor.requestRead();

    // Delimiter
    std::cout << "======================" << std::endl;

    // Iterate through threads
    for (std::map<int, pthread_t>::iterator i = connection_handler_threads.begin(); i != connection_handler_threads.end(); ++i)
    {
        std::cout << " Thread associated with socket " << i->first << std::endl;
    }
    // Delimiter
    std::cout << "======================" << std::endl;

    // Release read rights
    threads_monitor.releaseRead();
}

void Server::issueStop()
{

    // Issue a stop command to all running threads
    Server::stop_issued = true;

    // Request read rights
    Server::threads_monitor.requestRead();

    // Stop all communication with clients
    for (std::map<int, pthread_t>::iterator i = connection_handler_threads.begin(); i != connection_handler_threads.end(); ++i)
    {
        shutdown(i->first, SHUT_RDWR);
    }

    // Release read rights
    Server::threads_monitor.releaseRead();

    // Stop the main socket from receiving connections
    shutdown(Server::server_socket, SHUT_RDWR);
}