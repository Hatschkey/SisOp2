#include "Server.h"
#include "User.h"
#include "Group.h"

std::atomic<bool> Server::stop_issued;
typedef void (*command_function)(void);
std::map<std::string,command_function> Server::available_commands;

Server::Server(int N)
{
    // Validate N
    if (N <= 0)
        throw std::runtime_error("Invalid N, must be > 0");

    this->message_history = N;

    // Initialize shared data
    stop_issued = 0;

    // Initialize available server administrator commands
    available_commands.insert(std::make_pair("list users", &User::listUsers));
    available_commands.insert(std::make_pair("list groups", &Group::listGroups));
    available_commands.insert(std::make_pair("help", &Server::listCommands));

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

    // Admin instructions
    std::cout << "Server is ready to receive connections" << std::endl;
    Server::listCommands();

    // Wait for incoming connections
    // TODO Change from 'accept' to a non blocking method, so thread may be stopped safely
    int sockaddr_size = sizeof(struct sockaddr_in);
    int* new_socket;
    while(!stop_issued && (client_socket = accept(server_socket, (struct sockaddr*)&client_address, (socklen_t*)&sockaddr_size)) )
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
    login_payload* login_payload_buffer;        // Buffer for client login information
    std::string    message;                     // User chat message

    User* user   = NULL; // User the client is connected as
    Group* group = NULL; // Group the client is connected to

    while((read_bytes = recv(socket, client_message, PACKET_MAX, 0)) > 0)
    {
        // Decode received data into a packet structure
        packet* received_packet = (packet *)client_message;
        
        std::cout << "Received packet of type " << received_packet->type << " with " << read_bytes << " bytes" << std::endl;

        // Decide action according to packet type
        switch (received_packet->type)
        {
            case PAK_DAT:   // Data packet

                // Debug
                std::cout << "[" << user->username << " @ " << group->groupname << "] says: " << received_packet->_payload << std::endl;
                
                // Say the message to the group
                if (user != NULL && group != NULL)
                    user->say(received_packet->_payload, group->groupname);

                break;
            case PAK_CMD:   // Command packet (login)

                // Get user login information         
                login_payload_buffer = (login_payload*)received_packet->_payload;
                
                // TODO Debug messages
                std::cout << "Received login packet from " << login_payload_buffer->username << " @ " << login_payload_buffer-> groupname << std::endl;

                // Check if this group is already active, and create one if not
                if ( (group = Group::getGroup(login_payload_buffer->groupname)) == NULL)
                    group = new Group(login_payload_buffer->groupname);

                // Check if this user is logged in already, and create one if not
                if ( (user = User::getUser(login_payload_buffer->username)) == NULL)
                    user = new User(login_payload_buffer->username);

                // Try to join that group with this user
                if (user->joinGroup(group))
                {
                    // TODO If user was able to join group, show messages
                    // TODO Not sure if that code goes here or in Group methods
                }
                else
                {
                    // If user was not able to join group, refuse connection

                    // TODO Send message to client informing max of MAX_SESSIONS sessions
                    std::cout << "Connection was refused due to exceding MAX_SESSIONS: " << MAX_SESSIONS << std::endl;

                    // Reject connection
                    close(socket);

                    // Free received argument pointer
                    free(arg);

                    // Exit
                    pthread_exit(NULL);
                }

                break;
            default:
                std::cerr << "Unknown packet type received" << std::endl;
                break;
        }

        // Clear buffer to receive new packets
        for (int i=0; i < PACKET_MAX; i++) client_message[i] = '\0';
    }

    // Debug message
    std::cout << "[" << user->username << " @ " << group->groupname << "] disconnected" << std::endl;

    // Leave group with this user
    user->leaveGroup(group);

    // Free received argument pointer
    free(arg);

    // Exit
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

int Server::sendPacket(int socket, int packet_type, char* payload, int payload_size)
{
    int packet_size = -1;  // Size of packet to be sent
    int bytes_sent = -1;   // Number of bytes sent to the client

    // Prepare packet
    packet* data = (packet*)malloc(sizeof(*data) + sizeof(char) * payload_size);
    data->type      = PAK_CMD; // Signal that a data packet is being sent
    data->sqn       = 1; // TODO Keep track of the packet sequence numbers
    data->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp
    data->length = payload_size;    // Update payload size
    memcpy((char*)data->_payload, payload, payload_size); // Copy payload to packet

    // Calculate packet size
    packet_size = sizeof(*data) + (sizeof(char) * payload_size);

    // Send packet
    if ( (bytes_sent = send(socket, data, packet_size, 0)) <= 0)
        throw std::runtime_error("Unable to send message to server");

    // Free memory used for packet
    free(data);

    // Return number of bytes sent
    return bytes_sent;
}