#include "Client.h"

std::atomic<bool> Client::server_down;
std::atomic<bool> Client::stop_issued;
int Client::server_socket;
int Client::server_port;
int Client::listen_port;

pthread_t Client::input_handler_thread;
pthread_t Client::keep_alive_thread;
pthread_t Client::ui_update_thread;
pthread_t Client::election_listener_thread;

std::string Client::username;
RW_Monitor Client::socket_monitor;

// Election listener
int Client::ElectionListener::server_socket;
int Client::ElectionListener::listen_port;

Client::ElectionListener::ElectionListener(int N)
{
    if (N <= 0)
        throw std::runtime_error("Invalid N, must be > 0");

    // Initialize shared data
    stop_issued = 0;
    listen_port = N;

    setupConnection();
}

Client::ElectionListener::~ElectionListener()
{
    close(server_socket);
}

void Client::ElectionListener::listenConnections()
{
    int socket = -1; // Socket assigned to the incomming connection

    // Set passive listen socket
    if (listen(server_socket, 0) < 0)
        throw std::runtime_error(appendErrorMessage("Error setting socket as passive listener"));

    // Wait for incoming connections
    int sockaddr_size = sizeof(struct sockaddr_in);
    while (!stop_issued && (socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t *)&sockaddr_size)) > 0)
    {
        // Update the server socket
        Client::server_socket = socket;
        this->server_address = client_address;
        Client::server_port = client_address.sin_port;

        // Set server as reconnected
        server_down = false;
    }
}

void Client::ElectionListener::setupConnection()
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

//

Client::Client(std::string username, std::string groupname, std::string server_ip, std::string server_port, std::string listen_port)
{
    // Validate username
    if (!std::regex_match(username, std::regex(NAME_REGEX)))
        throw std::invalid_argument("Invalid username format");

    // Valide groupname
    if (!std::regex_match(groupname, std::regex(NAME_REGEX)))
        throw std::invalid_argument("Invalid group name format");

    // Validate IP
    if (!std::regex_match(server_ip, std::regex(IP_REGEX)))
        throw std::invalid_argument("Invalid IP format");

    // Validate server port
    if (!std::regex_match(server_port, std::regex(PORT_REGEX)))
        throw std::invalid_argument("Invalid Server Port format");

    // Validate listen port
    if (!std::regex_match(listen_port, std::regex(PORT_REGEX)))
        throw std::invalid_argument("Invalid Listen Port format");

    // Initialize values
    this->username = username;
    this->groupname = groupname;
    this->server_ip = !server_ip.compare("localhost") ? "127.0.0.1" : server_ip;
    this->server_port = stoi(server_port);
    this->server_socket = -1;

    // Start election listener
    Client::listen_port = stoi(listen_port);
    pthread_create(&election_listener_thread, NULL, startElectionListener, reinterpret_cast<void *>(Client::listen_port));

    // Set atomic flags as false
    stop_issued = false;
    server_down = false;

    // Initialize client interface
    ClientInterface::init(groupname, username);
};

Client::~Client()
{
    // Finalize client interface
    ClientInterface::destroy();

    // Close election listener
    pthread_join(Client::input_handler_thread, NULL);

    // Close server socket if it is still open
    if (server_socket > 0)
        close(server_socket);
};

void Client::setupConnection()
{
    message_record *login_record; // Record for sending login packet

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error(appendErrorMessage("Error during socket creation"));

    // Fill server socket address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = inet_addr(server_ip.c_str());

    // Try to connect to remote server
    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        throw std::runtime_error(appendErrorMessage("Error connecting to server"));

    // Prepare message record with login information
    login_record = CommunicationUtils::composeMessage(username, std::string(groupname), LOGIN_MESSAGE, (uint16_t)Client::listen_port);

    // Sends the command packet to the server
    CommunicationUtils::sendPacket(server_socket, PAK_COMMAND, (char *)login_record, sizeof(*login_record) + login_record->length);

    // Free login record
    free(login_record);

    // Start user input getter thread
    pthread_create(&input_handler_thread, NULL, handleUserInput, NULL);

    // Start keep-alive thread
    pthread_create(&keep_alive_thread, NULL, keepAlive, NULL);

    // Start UI updater thread
    pthread_create(&ui_update_thread, NULL, ClientInterface::updateUI, NULL);
};

void Client::getMessages()
{
    int read_bytes = -1;              // Number of bytes read from the header
    char server_message[PACKET_MAX];  // Buffer for message sent from server
    message_record *received_message; // Pointer to a message record, used to decode received packet payload
    packet *received_packet;

    char message_time[9];     // Timestamp of the message
    std::string chat_message; // Final composed chat message string, printed to the interface
    std::string username;     // Name of the user who sent the message

    // Clear buffer to receive new packets
    bzero((void *)server_message, PACKET_MAX);

    // While reconnect attempts are sucessful
    while (!server_down)
    {
        // Wait for messages from the server
        while (!stop_issued && (read_bytes = CommunicationUtils::receivePacket(server_socket, server_message, PACKET_MAX)) > 0)
        {
            // Decode message into packet format
            received_packet = (packet *)server_message;

            // Try to read the rest of the payload from the socket stream
            switch (received_packet->type)
            {
            case PAK_DATA: // Data packet (messages)

                // Decode payload into a message record
                received_message = (message_record *)received_packet->_payload;

                // If message was sent by this user, change display name to "You"
                if (strcmp(received_message->username, this->username.c_str()) == 0)
                {
                    sprintf(received_message->username, "%s", "You");
                }
                // If not, add brackets to display name: [username]
                else
                {
                    username = std::string("[") + received_message->username + "]";
                    sprintf(received_message->username, "%s", username.c_str());
                }

                // Get time into a readable format
                strftime(message_time, sizeof(message_time), "%H:%M:%S", std::localtime((time_t *)&received_message->timestamp));

                if (received_message->type == SERVER_MESSAGE)
                {
                    chat_message = message_time + std::string(" ") + received_message->_message;
                }
                else if (received_message->type == USER_MESSAGE)
                {
                    chat_message = message_time + std::string(" ") + received_message->username + ": " + received_message->_message;
                }

                // Display message
                ClientInterface::printMessage(chat_message);
                break;

            case PAK_SERVER_MESSAGE: // Server broadcast message for clients login/logout

                // Decode payload into a message record
                received_message = (message_record *)received_packet->_payload;

                // Get time into a readable format
                strftime(message_time, sizeof(message_time), "%H:%M:%S", std::localtime((time_t *)&received_message->timestamp));

                // Display message
                chat_message = message_time + std::string(" ") + std::string(received_message->_message);
                ClientInterface::printMessage(chat_message);

                break;

            case PAK_COMMAND: // Command packet (disconnect)

                // Decode payload into a message record
                received_message = (message_record *)received_packet->_payload;

                // Show message to user
                ClientInterface::printMessage(received_message->_message);

                // Stop the client application
                stop_issued = true;
                read_bytes = 0;
                break;

            case PAK_NEW_SERVER:
                ClientInterface::printMessage("Connected to a new server");
                break;
            default: // Unknown packet
                ClientInterface::printMessage("Received unkown packet from server");
                break;
            }

            // Clear buffer to receive new packets
            bzero((void *)server_message, PACKET_MAX);
        }

        // Set server as down
        server_down = true;

        // If not stopping because of client action
        if (!stop_issued)
        {
            // Wait
            sleep(USER_RECONNECT_TIMEOUT);
        }
    }

    // Reset socket
    ClientInterface::printMessage("Connection closed");

    // Signal input handler to stop
    stop_issued = true;

    // Wait for input handler thread to finish
    pthread_join(Client::input_handler_thread, NULL);

    // Wake up the keep-alive thread from it's sleep
    pthread_cancel(Client::keep_alive_thread);
    pthread_join(Client::keep_alive_thread, NULL);

    // Finish the UI updater thread
    pthread_cancel(Client::ui_update_thread);
    pthread_join(Client::ui_update_thread, NULL);
};

void *Client::handleUserInput(void *arg)
{
    message_record *message;

    // Get user messages to be sent until Ctrl D is pressed
    char user_message[MESSAGE_MAX + 1];
    do
    {
        if (wgetnstr(ClientInterface::inptscr, user_message, MESSAGE_MAX) == ERR)
            stop_issued = true;
        if (!stop_issued)
        {
            try
            {
                // Reset input area below the screen
                ClientInterface::resetInput();

                if (strlen(user_message) > 0)
                {
                    // Compose message
                    message = CommunicationUtils::composeMessage(username, std::string(user_message), USER_MESSAGE);

                    // Request write rights
                    socket_monitor.requestWrite();

                    // Send message to server
                    CommunicationUtils::sendPacket(server_socket, PAK_DATA, (char *)message, sizeof(*message) + message->length);

                    // Release write rights
                    socket_monitor.releaseWrite();

                    // Free composed message
                    free(message);
                }
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << e.what() << std::endl;
            }
        }

        bzero((void *)user_message, MESSAGE_MAX + 1);
    } while (!stop_issued);

    // Signal server-litening thread to stop
    stop_issued = true;

    // Close listening socket
    shutdown(server_socket, SHUT_RDWR);

    // End with no return value
    pthread_exit(NULL);
};

void *Client::startElectionListener(void *arg)
{

    int port = reinterpret_cast<long>(arg);
    ElectionListener *listener = new ElectionListener(port);
    listener->listenConnections();
    delete listener;
    pthread_exit(NULL);
}

void *Client::keepAlive(void *arg)
{
    // Useless message that will be sent to server to keep connection going
    char keep_alive = 'x';

    while (!stop_issued)
    {
        // Sleep for SLEEP_TIME seconds between attempting to send messages to the server
        sleep(SLEEP_TIME);

        if (!stop_issued && !server_down)
        {
            // Request write rights
            socket_monitor.requestWrite();

            // Send message
            CommunicationUtils::sendPacket(Client::server_socket, PAK_KEEP_ALIVE, &keep_alive, sizeof(keep_alive));

            // Release write rights
            socket_monitor.releaseWrite();
        }
    }

    // Exit
    pthread_exit(NULL);
}
