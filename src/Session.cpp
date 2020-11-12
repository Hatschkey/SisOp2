#include "Session.h"

Session::Session(std::string username, std::string groupname, int socket)
{
    // Variables for if the user has too many sessions
    std::string message;
    message_record *dc = NULL;

    // Initial values
    this->socket = socket;
    this->user = NULL;
    this->group = NULL;

    // Attempt to join that group with that user
    if (!Group::joinByName(username, groupname, &this->user, &this->group, this))
    {
        // Compose disconnect message
        message = "Connection was refused: exceeds MAX_SESSIONS (" + std::to_string(MAX_SESSIONS) + ")";

        // Compose message record
        dc = CommunicationUtils::composeMessage(username, message, PAK_SERVER_MESSAGE);

        // Send message record to client
        sendPacket(this->socket, PAK_COMMAND, (char *)dc, sizeof(*dc) + dc->length);
    }
}

Session::~Session()
{
    // Leave the group with the user
    this->user->leaveGroup(this);

    // Close the socket
    close(this->socket);
}

bool Session::isOpen()
{
    return this->user != NULL ? true : false;
}

int Session::getId()
{
    return this->socket;
}

Group *Session::getGroup()
{
    return this->group;
}

User *Session::getUser()
{
    return this->user;
}

void Session::setSocket(int socket)
{
    // Update user side socket
    this->user->updateSession(this->socket, socket);

    // Update this side's socket
    this->socket = socket;
}

int Session::sendHistory(int N)
{
    char read_buffer[PACKET_MAX * N]; // Buffer for messages
    int message_count;                // How many messages were actually read
    int offset = 0;                   // Current ofset in read buffer
    char send_buffer[PACKET_MAX];     // Buffer for sending messages
    message_record *message;          // Composed message that will be sent

    // Initialize message records buffer
    bzero((void *)read_buffer, PACKET_MAX * N);

    // Recover message history
    message_count = this->group->recoverHistory(read_buffer, N, user);

    // Iterate through the buffer
    for (int i = 0; i < message_count; i++)
    {
        // Decode the buffer data into a message record
        message = (message_record *)(read_buffer + offset);

        // Clear the send buffer and copy the new data into it
        bzero((void *)send_buffer, PACKET_MAX);
        memcpy(send_buffer, read_buffer + offset, sizeof(message_record) + message->length);

        // Send the old message to the connected client
        sendPacket(socket, PAK_DATA, send_buffer, sizeof(message_record) + message->length);

        // Go forward in the buffer
        offset += sizeof(message_record) + message->length;
    }

    return message_count;
}

void Session::messageGroup(message_record *message)
{
    // Update user's last seen variable
    this->user->setLastSeen();

    // Send message to the group
    if (this->group != NULL)
        this->group->post(message->_message, this->user->username, USER_MESSAGE);
}

void Session::messageClient(message_record *message, int packet_type)
{
    // Calculate message size
    int size = sizeof(*message) + message->length;

    // Send
    CommunicationUtils::sendPacket(this->socket, packet_type, (char *)message, size);
}
