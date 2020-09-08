#include "Client.h"

std::atomic<bool> Client::stop_issued;
int Client::server_socket;

Client::Client(std::string username, std::string groupname, std::string server_ip, std::string server_port)
{
    // Validate username
    if ( !std::regex_match(username,std::regex(NAME_REGEX)) )
        throw std::invalid_argument("Invalid username format");

    // Valide groupname
    if ( !std::regex_match(groupname, std::regex(NAME_REGEX)))
        throw std::invalid_argument("Invalid group name format");

    // Validate IP
    if (!std::regex_match(server_ip, std::regex(IP_REGEX)))
        throw std::invalid_argument("Invalid IP format");

    // TODO Validate port maybe?

    // Initialize values
    this->username = username;
    this->groupname = groupname;
    this->server_ip = server_ip;
    this->server_port = stoi(server_port);
    this->server_socket = -1;

    // Set atomic flag as false
    stop_issued = false;
};

Client::~Client()
{
    // Close server socket if it is still open
    if (server_socket > 0)
        close(server_socket);

};

void Client::setupConnection()
{
    // Create socket
    if ( (server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error("Error during socket creation");
    
    // Fill server socket address
    server_address.sin_family = AF_INET;           
    server_address.sin_port   = htons(server_port);
    server_address.sin_addr.s_addr = inet_addr(server_ip.c_str());
   
    // Try to connect to remote server
    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        throw std::runtime_error("Error connecting to server");

    // Send the first packet for login
    sendLoginPacket();

    // Start message getter thread
    pthread_create(&server_listener_thread,NULL, getMessages, NULL);

};

void Client::handleUserInput()
{
    // Flush stdin so no empty message is sent
    fflush(stdin);

    // Get user messages to be sent until Ctrl D is pressed
    std::string user_message;
    do
    {
        
        std::cout << "Say something: ";
        try
        {
            // Don't send empty messages
            if (!user_message.empty())
                sendMessagePacket(user_message);
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << e.what() << std::endl;
        }
        
    }
    while(std::getline(std::cin, user_message));

    // Signal server-litening thread to stop
    stop_issued = true;

    // Wait for thread to finish
    pthread_join(server_listener_thread,NULL);
};

void *Client::getMessages(void* arg)
{
    int read_bytes = -1;
    //char server_message[PACKET_MAX];

    // TODO Wait for messages from the server
    while(!stop_issued)
    {

    }
    // If server closes connection
    if (read_bytes == 0)
    {
        std::cout << "Connection closed by server" << std::endl;
    }

    pthread_exit(NULL);
};

int Client::sendLoginPacket()
{
    int payload_size = -1; // Size of login payload
    int packet_size = -1;  // Size of packet to be sent
    int bytes_sent = -1;   // Number of bytes sent to the remote server

    // Prepare login content payload
    login_payload lp;
    sprintf(lp.username, "%s", username.c_str());
    sprintf(lp.groupname, "%s", groupname.c_str());
    payload_size = sizeof(lp);

    // Prepare packet 
    packet* login_packet = (packet*)malloc(sizeof(*login_packet) + sizeof(char) * payload_size);
    login_packet->type      = PAK_CMD; // Signal that a command packet is being sent
    login_packet->sqn       = 0;       // Sequence number is 0 for the first packet
    login_packet->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp 
    login_packet->length    = payload_size; // Update payload size
    memcpy((void*)login_packet->_payload, (void*)&lp, payload_size);  // Load login payload

    // Calculate packet size
    packet_size = sizeof(*login_packet) +  payload_size;

    // Send packet
    if ( (bytes_sent = send(server_socket, login_packet, packet_size, 0)) <= 0)
        throw std::runtime_error("Unable to send login packet to server");
    
    // Free memory used for packet
    free(login_packet);

    // Return number of bytes sent
    return bytes_sent;
};

int Client::sendMessagePacket(std::string message)
{
    int payload_size = -1; // Size of login payload
    int packet_size = -1;  // Size of packet to be sent
    int bytes_sent = -1;   // Number of bytes sent to the remote server

    // Prepare message payload
    const char* payload = message.c_str();
    payload_size = strlen(payload) + 1;

    // Prepare packet
    packet* data = (packet*)malloc(sizeof(*data) + sizeof(char) * payload_size);
    data->type      = PAK_DAT; // Signal that a data packet is being sent
    data->sqn       = 1; // TODO Keep track of the packet sequence numbers
    data->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp
    data->length = payload_size;    // Upload payload size
    sprintf((char*)data->_payload, "%s",payload); // Load message payload
    
    // Calculate packet size
    packet_size = sizeof(*data) + (sizeof(char) * payload_size);

    // Send packet
    if ( (bytes_sent = send(server_socket, data, packet_size, 0)) <= 0)
        throw std::runtime_error("Unable to send message to server");

    // Free memory used for packet
    free(data);

    // Return number of bytes sent
    return bytes_sent;
};