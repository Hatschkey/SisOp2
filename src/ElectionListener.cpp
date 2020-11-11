#include "ElectionListener.h"

std::atomic<bool> ElectionListener::stop_issued;
int ElectionListener::server_socket;
int ElectionListener::listen_port;

ElectionListener::ElectionListener(int N)
{
    if (N <= 0)
        throw std::runtime_error("Invalid N, must be > 0");

    // Initialize shared data
    stop_issued = 0;
    listen_port = N;

    setupConnection();
}

ElectionListener::~ElectionListener()
{
    close(server_socket);
}

void ElectionListener::listenConnections()
{
    int client_socket = -1; // Socket assigned to the incomming connection

    // Create struct for socket timeout and set USER_TIMEOUT seconds
    struct timeval timeout_timer;
    timeout_timer.tv_sec = USER_TIMEOUT;
    timeout_timer.tv_usec = 0;

    // Set passive listen socket
    if (listen(server_socket, 0) < 0)
        throw std::runtime_error(appendErrorMessage("Error setting socket as passive listener"));

    // Wait for incoming connections
    int sockaddr_size = sizeof(struct sockaddr_in);
    while (!stop_issued && (client_socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t *)&sockaddr_size)) > 0)
    {
        // Set the keep-alive timer on the socket
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_timer, sizeof(timeout_timer)) < 0)
            throw std::runtime_error(appendErrorMessage("Error setting socket options"));

        handleConnection(client_socket);
    }
}

void ElectionListener::handleConnection(int socket)
{
    int read_bytes = -1;             // Number of bytes read from the message
    char server_message[PACKET_MAX]; // Buffer for client message, maximum of PACKET_MAX bytes

    //while ((read_bytes = recv(socket, server_message, PACKET_MAX, 0)) > 0)
    while (!stop_issued && (read_bytes = CommunicationUtils::receivePacket(socket, server_message, PACKET_MAX)) > 0)
    {
        packet *received_packet = (packet *)server_message;

        if (received_packet->type == 1)
        {
            new_server_addr *new_addr = (new_server_addr *)(received_packet->_payload);
            //TODO - Notify Client.cpp to change server
        }

        // Clear the message buffer
        bzero((void *)server_message, PACKET_MAX);
    }

    close(socket);
}

void ElectionListener::setupConnection()
{
    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error(appendErrorMessage("Error during socket creation"));

    // Prepare server socket address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(listen_port);

    // Set socket options
    int yes = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        throw std::runtime_error(appendErrorMessage("Error setting socket options"));

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        throw std::runtime_error(appendErrorMessage("Error during socket bind"));
}
