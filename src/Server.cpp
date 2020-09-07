#include "Server.h"

managed_data_t Server::shared_data;

Server::Server(int N)
{
    // Validate N
    if (N <= 0)
        throw std::runtime_error("Invalid N, must be > 0");

    this->message_history = N;

    // Initialize shared data
    shared_data.stop_issued = 0;

    // Setup socket
    setupConnection();
}

Server::~Server()
{
    // Close opened sockets
    close(server_socket);
    close(client_socket);

}

void Server::listenConnections()
{
    // Set passive listen socket
    if ( listen(server_socket, 3) < 0)
        throw std::runtime_error("Error setting socket as passive listener");

    // Spawn thread for listening to administrator commands
    pthread_create(&command_handler_thread, NULL, handleCommands, NULL);

    // Wait for incoming connections
    std::cout << "Server is ready to receive connections" << std::endl;
    
    // TODO Change from 'accept' to a non blocking method, so thread may be stopped safely
    int sockaddr_size = sizeof(struct sockaddr_in);
    int* new_socket;
    while(!shared_data.stop_issued && (client_socket = accept(server_socket, (struct sockaddr*)&client_address, (socklen_t*)&sockaddr_size)) )
    {
        std::cout << "New connection accepted from client " << client_socket << std::endl;
        
        // Start new thread for new connection
        pthread_t comm_thread;
        
        // Get reference to client socket
        new_socket = (int*)malloc(1);
        *new_socket = client_socket;
        
        // Spawn new thread for handling that client
        if (pthread_create( &comm_thread, NULL, handleConnection, (void*)new_socket) < 0)
        {
            // Close socket if no thread was created
            std::cerr << "Could not create thread for socket " << client_socket << std::endl;
            close(client_socket);
        }
          
        // Add thread to list of connection handlers
        connection_handler_threads.push_back(comm_thread);

        // Free new socket pointer
        //free(new_socket);
    }

    // Wait for all threads to finish
    for (std::vector<pthread_t>::iterator i = connection_handler_threads.begin(); i != connection_handler_threads.end(); ++i)
    {
        std::cout << "Waiting for client communication to end..." << std::endl;
        pthread_t* ref = &(*i);
        pthread_join(*ref, NULL);
    }

    // TODO Wait for Group and Message managers to end

}

void *Server::handleCommands(void* arg)
{
    // Get administrator commands until Ctrl D is pressed
    std::string command;
    while(std::getline(std::cin, command))
    {
        if (!command.compare("list"))
            listUsers();
    }

    // TODO MUTEX Signal all other threads to end
    Server::shared_data.stop_issued = 1;

    pthread_exit(NULL);
}

void *Server::handleConnection(void* arg)
{

    int            socket = *(int*)arg;         // Client socket
    int            read_bytes = -1;             // Number of bytes read from the message
    char           client_message[PACKET_MAX];  // Buffer for client message, maximum of PACKET_MAX bytes
    login_payload* login_payload_buffer;        // Buffer for client login information
    std::string    message;                     // User chat message

    user_t* user   = NULL; // User the client is connected as
    group_t* group = NULL; // Group the client is connected to
 
    std::cout << "Created new connection handler for the client " << socket << std::endl;

    while((read_bytes = recv(socket, client_message, PACKET_MAX, 0)) > 0)
    {
        // Decode received data into a packet structure
        packet* received_packet = (packet *)client_message;
        
        std::cout << "Received packet of type " << received_packet->type << " with " << read_bytes << " bytes" << std::endl;

        // Decide action according to packet type
        switch (received_packet->type)
        {
            case PAK_DAT:   // Data packet

                // TODO inform message and group threads, the below code is only for debug purposes
                std::cout << "[" << user->username << " @ " << group->groupname << "] says: " << received_packet->_payload << std::endl;

                break;
            case PAK_CMD:   // Command packet (login)

                // Get user login information         
                login_payload_buffer = (login_payload*)received_packet->_payload;
                
                // TODO Debug messages
                std::cout << "Received login packet from " << login_payload_buffer->username << " @ " << login_payload_buffer-> groupname << std::endl;

                // Check if this user is logged in already
                if ( (user = getUser(login_payload_buffer->username)) == NULL)
                {
                    // If user was not found, create a new one
                    user = (user_t*)malloc(sizeof(user_t));
                    user->active_sessions = 1;
                    user->last_seen = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                    sprintf(user->username, "%s", login_payload_buffer->username);

                    // Add new user to list
                    addUser(user);
                }
                else
                {
                    
                    // If user was found, check for session count
                    if (user->active_sessions >= MAX_SESSIONS)
                    {
                        // Reject connection
                        close(socket);

                        // TODO Send message to client informing connection was rejected

                        // Free received argument pointer
                        free(arg);

                        // Exit
                        pthread_exit(NULL);
                    }
                    
                    // Increase user session count
                    user->active_sessions++;
                }

                // TODO Check if this group is currently active, and check for message history, for now assuming no
                group = (group_t*)malloc(sizeof(group_t));
                sprintf(group->groupname, "%s", login_payload_buffer->groupname);

                // TODO Send the last N² group messages to the client

                break;
            default:
                std::cerr << "Unknown packet type received" << std::endl;
                break;
        }

        // Clear buffer to receive new packets
        for (int i=0; i < PACKET_MAX; i++) client_message[i] = '\0';
    }

    // On client disconnect
    if (read_bytes == 0) 
    {
        std::cout << "[" << user->username << " @ " << group->groupname << "] disconnected" << std::endl;

        // Decrease user active session count
        user->active_sessions--;
        
        // If no active sessions for the user are left, free user space
        if (user->active_sessions == 0)
        {
            // Remove user from list
            removeUser(user->username);

            // Free user structure
            free(user);
        }
    }

    // TODO Check if this was last active user in group, and free group space
    free(group);

    // Free received argument pointer
    free(arg);

    pthread_exit(NULL);
}

void Server::setupConnection()
{
    // Create socket
    if ( (server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error("Error during socket creation");

    // Prepare server socket address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);

    // Set socket options
    int yes = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        throw std::runtime_error("Error setting socket options");

    // Bind socket
    if ( bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        throw std::runtime_error("Error during socket bind");
    
}

user_t* Server::getUser(std::string username)
{

    try
    {
        return shared_data.active_users.at(username);
    }
    catch(const std::out_of_range& e)
    {
        return NULL;
    }
        
};

void Server::addUser(user_t* user)
{
    // Insert user in list
    shared_data.active_users.insert(std::make_pair(user->username,user));
}

int Server::removeUser(std::string username)
{
    // Remove user from map
    return shared_data.active_users.erase(username);
}

void Server::listUsers()
{
    // Iterate map listing users
    for (std::map<std::string,user_t*>::iterator i = shared_data.active_users.begin(); i != shared_data.active_users.end(); ++i)
    {
        std::cout << "Username: " << i->second->username << std::endl;
        std::cout << "Active sessions: " << i->second->active_sessions << std::endl;
        std::cout << "Last seen: " << i->second->last_seen << std::endl;
    }
    
}