#include "Server.h"
#include "User.h"
#include "Group.h"

std::atomic<bool> Server::stop_issued;
typedef void (*command_function)(void);
std::map<std::string,command_function> Server::available_commands;
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
    available_commands.insert(std::make_pair("list threads",&Server::listThreads));
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
    int client_socket = -1;
    // Set passive listen socket
    if ( listen(server_socket, 3) < 0)
        throw std::runtime_error(appendErrorMessage("Error setting socket as passive listener"));

    // Spawn thread for listening to administrator commands
    pthread_create(&command_handler_thread, NULL, handleCommands, NULL);

    // Admin instructions
    std::cout << "Server is ready to receive connections" << std::endl;
    Server::listCommands();

    // Wait for incoming connections
    int sockaddr_size = sizeof(struct sockaddr_in);
    int* new_socket;
    while(!stop_issued && (client_socket = accept(server_socket, (struct sockaddr*)&client_address, (socklen_t*)&sockaddr_size)) > 0 )
    {
        //std::cout << "New connection accepted from client " << client_socket << std::endl;

        // Start new thread for new connection
        pthread_t comm_thread;

        // Get reference to client socket
        new_socket = (int*)malloc(sizeof(int));
        *new_socket = client_socket;

        // Spawn new thread for handling that client
        if (pthread_create( &comm_thread, NULL, handleConnection, (void*)new_socket) < 0)
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

    // Wait for all threads to finish
    for (std::map<int, pthread_t>::iterator i = connection_handler_threads.begin(); i != connection_handler_threads.end(); ++i)
    {
        std::cout << "Waiting for client communication to end on socket " << i->first << "..." << std::endl;
        pthread_t* ref = &(i->second);
        pthread_join(*ref, NULL);
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
    for (std::map<std::string,command_function>::iterator i = Server::available_commands.begin(); i != Server::available_commands.end(); ++i)
    {
        std::cout << " " << i->first << std::endl;
    }
}

void *Server::handleCommands(void* arg)
{
    // Get administrator commands until Ctrl D is pressed or 'stop' is typed
    std::string command;
    while(!stop_issued && std::getline(std::cin, command))
    {
        try
        {
            // Execute administrator command
            available_commands.at(command)();
        }
        catch(std::out_of_range& e)
        {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }

    // Signal all other threads to end
    if (!stop_issued) Server::issueStop();

    pthread_exit(NULL);
}

void *Server::handleConnection(void* arg)
{
    int            socket = *(int*)arg;         // Client socket
    int            read_bytes = -1;             // Number of bytes read from the message
    char           client_message[PACKET_MAX];  // Buffer for client message, maximum of PACKET_MAX bytes
    char           server_message[PACKET_MAX];  // Buffer for messages the server will send to the client, max PACKET_MAX bytes
    login_payload* login_payload_buffer;        // Buffer for client login information
    std::string    message;                     // User chat message
    std::string    username;                    // Connected user's name
    std::string    groupname;                   // Connected group's name

    char message_records[PACKET_MAX*Server::message_history];
    message_record* read_message;
    int read_messages = 0;
    int offset = 0;

    User* user   = NULL; // User the client is connected as
    Group* group = NULL; // Group the client is connected to

    while((read_bytes = recv(socket, client_message, PACKET_MAX, 0)) > 0)
    {
        // Decode received data into a packet structure
        packet* received_packet = (packet *)client_message;

        // Decide action according to packet type
        switch (received_packet->type)
        {
            case PAK_DATA:   // Data packet            
                // Say the message to the group
                if (user != NULL && group != NULL && !stop_issued)
                    user->say(received_packet->_payload, group->groupname);
                break;

            case PAK_COMMAND:   // Command packet (login)
                // Get user login information
                login_payload_buffer = (login_payload*)received_packet->_payload;              

                // Try to join that group with this user
                if (Group::joinByName(login_payload_buffer->username, login_payload_buffer->groupname, &user, &group, socket) != 0)
                {
                    // Update connected user and groupname
                    groupname = group->groupname;
                    username = user->username;

                    // Initialize message records buffer
                    bzero(message_records, PACKET_MAX * Server::message_history);

                    // Recover message history for this user
                    read_messages = group->recoverHistory(message_records, Server::message_history, user);

                    // Iterate through the buffer
                    for (int i = 0; i < read_messages; i++)
                    {
                        // Decode the buffer data into a message record
                        read_message = (message_record*)(message_records + offset);

                        // Clear the buffer and copy the new data into it
                        bzero(server_message, PACKET_MAX);
                        memcpy(server_message, message_records + offset, sizeof(message_record) + read_message->length);

                        // Send the old message to the connected client
                        sendPacket(socket, PAK_DATA, server_message, sizeof(message_record) + read_message->length);

                        // Go forward in the buffer
                        offset += sizeof(message_record) + read_message->length;
                    }

                    if (!stop_issued && user->getSessionCount() == 1) 
                    {
                        message = "User [" + user->username + "] has joined.";
                        group->broadcastMessage(message ,user->username);
                    }
                }
                else
                {
                    // If user was not able to join group, refuse connection
                    // TODO Send message in some type of standard struct
                    sprintf(server_message, "%s %d", "Connection was refused due to exceding MAX_SESSIONS: ", MAX_SESSIONS);
                    sendPacket(socket, PAK_COMMAND, server_message, sizeof(char)*strlen(server_message) + 1);

                    // Reject connection
                    close(socket);

                    // Free received argument pointer
                    free(arg);

                    // Exit
                    pthread_exit(NULL);
                }
                break;

            default:
                break;
        }

        // Clear the buffers
        bzero(client_message, PACKET_MAX);
        bzero(message_records, PACKET_MAX * Server::message_history);
    }
    
    // Leave group with this user
    user->leaveGroup(group, socket);
    
    // Request read rights
    User::active_users_monitor.requestRead();
    Group::active_groups_monitor.requestRead();

    // Check if the user doesn't exist, the group still exists, and server was not stopped
    if (User::active_users.find(username) == User::active_users.end()   &&
        Group::active_groups.find(groupname) != Group::active_groups.end() &&
        !stop_issued)
    {
        message = "User [" + username + "] has disconnected.";
        group->broadcastMessage(message ,username);
    }

    // Release read rights
    User::active_users_monitor.releaseRead();
    Group::active_groups_monitor.releaseRead();

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
    if ( (server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
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
    if ( bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
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
        std::cout << " Thread associated with socket " << i->first <<std::endl;
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