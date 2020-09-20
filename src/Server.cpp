#include "Server.h"
#include "User.h"
#include "Group.h"

std::atomic<bool> Server::stop_issued;
typedef void (*command_function)(void);
std::map<std::string,command_function> Server::available_commands;
int Server::message_history;

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
    // TODO Change from 'accept' to a non blocking method, so thread may be stopped safely
    int sockaddr_size = sizeof(struct sockaddr_in);
    int* new_socket;
    while(!stop_issued && (client_socket = accept(server_socket, (struct sockaddr*)&client_address, (socklen_t*)&sockaddr_size)) )
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
        std::cout << "Waiting for client communication to end..." << std::endl;
        pthread_t* ref = &(i->second);
        pthread_join(*ref, NULL);
    }

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
    // Get administrator commands until Ctrl D is pressed
    std::string command;
    while(std::getline(std::cin, command))
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
    stop_issued = true;

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

        //std::cout << "Received packet of type " << received_packet->type << " with " << read_bytes << " bytes" << std::endl;

        // Decide action according to packet type
        switch (received_packet->type)
        {
            case PAK_DAT:   // Data packet
                // Debug
                //std::cout << "[" << user->username << " @ " << group->groupname << "] says: " << received_packet->_payload << std::endl;

                // Say the message to the group
                if (user != NULL && group != NULL)
                    user->say(received_packet->_payload, group->groupname);
                break;

            case PAK_CMD:   // Command packet (login)
                // Get user login information
                login_payload_buffer = (login_payload*)received_packet->_payload;

                // TODO Debug messages
                //std::cout << "Received login packet from " << login_payload_buffer->username << " @ " << login_payload_buffer-> groupname << std::endl;

                // Check if this group is already active, and create one if not
                if ( (group = Group::getGroup(login_payload_buffer->groupname)) == NULL)
                    group = new Group(login_payload_buffer->groupname);

                // Check if this user is logged in already, and create one if not
                if ( (user = User::getUser(login_payload_buffer->username)) == NULL)
                    user = new User(login_payload_buffer->username);

                // Try to join that group with this user
                if (user->joinGroup(group, socket) != 0)
                {
                    // Recover message history for this user
                    read_messages = group->recoverHistory(message_records, Server::message_history, user);

                    for (int i = 0; i < read_messages; i++)
                    {
                        read_message = (message_record*)(message_records + offset);

                        memcpy(server_message, message_records + offset, sizeof(message_record) + read_message->length);

                        sendPacket(socket, PAK_DAT, server_message, sizeof(message_record) + ((message_record*)server_message)->length);

                        //std::cout << ((message_record*)server_message)->username << std::endl;
                        //std::cout << ((message_record*)server_message)->timestamp << std::endl;
                        //std::cout << ((message_record*)server_message)->length << std::endl;
                        //std::cout << ((message_record*)server_message)->_message << std::endl;

                        offset += sizeof(message_record) + ((message_record*)server_message)->length;
                    }
                }
                else
                {
                    // If user was not able to join group, refuse connection
                    // TODO Send message in some type of standard struct
                    sprintf(server_message, "%s %d", "Connection was refused due to exceding MAX_SESSIONS: ", MAX_SESSIONS);
                    sendPacket(socket, PAK_CMD, server_message, sizeof(char)*strlen(server_message) + 1);

                    // Reject connection
                    close(socket);

                    // Free received argument pointer
                    free(arg);

                    // Exit
                    pthread_exit(NULL);
                }
                break;

            default:
                //std::cerr << "Unknown packet type received" << std::endl;
                break;
        }

        // Clear buffers to receive and send new packets
        for (int i=0; i < PACKET_MAX; i++)
        {
            client_message[i] = '\0';
            server_message[i] = '\0';
        }
    }

    // Debug message
    //std::cout << "[" << user->username << " @ " << group->groupname << "] disconnected" << std::endl;

    // Leave group with this user
    user->leaveGroup(group, socket);

    // Free received argument
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
    threads_monitor.requestRead();

    for (std::map<int, pthread_t>::iterator i = connection_handler_threads.begin(); i != connection_handler_threads.end(); ++i)
    {
        std::cout << "Thread associated with socket " << i->first <<std::endl;
    }

    threads_monitor.releaseRead();
}
