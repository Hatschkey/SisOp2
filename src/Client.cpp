#include "Client.h"

std::atomic<bool> Client::stop_issued;
int Client::server_socket;
pthread_t Client::input_handler_thread;

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

    // Initialize client interface
    ClientInterface::init(groupname);
};

Client::~Client()
{
    // Finalize client interface
    ClientInterface::destroy();

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

    // Start user input getter thread
    pthread_create(&input_handler_thread, NULL, handleUserInput, NULL);

};

void Client::getMessages()
{
    int read_bytes = -1;                // Number of bytes
    char server_message[PACKET_MAX];    // Buffer for message sent from server
    message_record* received_message;   // Pointer to a message record, used to decode received packet payload

    std::string chat_message; // Final composed chat message string, printed to the interface

    // Clear buffer to receive new packets
    for (int i=0; i < PACKET_MAX; i++) server_message[i] = '\0';
    
    // Wait for messages from the server
    while(!stop_issued && (read_bytes = recv(server_socket, server_message, PACKET_MAX, 0)) > 0)
    {
        // Decode message into packet format
        packet* received_packet = (packet*)server_message;

        switch(received_packet->type)
        {
            case PAK_DAT: // Data packet (messages)

                // Decode payload into a message record
                received_message = (message_record*)received_packet->_payload;

                // If message was sent by this user, change display name to "You"
                if (strcmp(received_message->username, this->username.c_str()) == 0)
                {
                    sprintf(received_message->username,"%s","You");
                }
                // If not, add brackets to display name: [username]
                else
                {
                    sprintf(received_message->username,"[%s]",received_message->username);
                }

                // Display message
                chat_message = std::ctime((time_t*)&received_message->timestamp) + std::string(" ") + received_message->username + ": " + received_message->_message;
                ClientInterface::printMessage(chat_message);

                break;
            case PAK_CMD: // Command packet (disconnect)

                ClientInterface::printMessage(received_packet->_payload);

                // Stop the client application
                stop_issued = true;
                read_bytes = 0;

                break;

            default: // Unknown packet 
                ClientInterface::printMessage("Received unkown packet from server");
                break;
        }

        // Clear buffer to receive new packets
        for (int i=0; i < PACKET_MAX; i++) server_message[i] = '\0';
    }
    // If server closes connection
    if (read_bytes == 0)
    {
        std::cout << "\nConnection closed." << std::endl;
    }

    // Wait for thread to finish
    pthread_join(Client::input_handler_thread,NULL);

};

void *Client::handleUserInput(void* arg)
{
    // Flush stdin so no empty message is sent
    fflush(stdin);

    // Get user messages to be sent until Ctrl D is pressed
    char user_message[MESSAGE_MAX];
    do
    {
        // Get user message
        // TODO Detect Ctrl D press instead of Ctrl C
        getstr(user_message);
        try
        {
            // Reset input area below the screen
            ClientInterface::resetInput();

            // If user pressed ctrl D
            if (user_message[0] == KEY_END)
            {
                // End program
                stop_issued = true;
            }
            else
            {    
                // Don't send empty messages
                if (strlen(user_message) > 0)
                    sendMessagePacket(std::string(user_message));
            }
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << e.what() << std::endl;
        }
        
    }
    while(!stop_issued);

    // Signal server-litening thread to stop
    stop_issued = true;

    // Close listening socket
    shutdown(server_socket, SHUT_RDWR);

    // End with no return value
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