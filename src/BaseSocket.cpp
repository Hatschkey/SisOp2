#include "BaseSocket.h"

std::string BaseSocket::appendErrorMessage(const std::string message)
{
    return message + " (" + std::string(strerror(errno)) + ")";
}

int BaseSocket::sendPacket(int socket, int packet_type, char* payload, int payload_size)
{
    // Calculate size of packet that will be sent
    int packet_size = sizeof(packet) + payload_size;
    int bytes_sent = -1;   // Number of bytes actually sent to the client

    // TODO Sequence numbers!

    // Prepare packet
    packet* data = (packet*)malloc(packet_size);          // Malloc memory
    bzero(data, packet_size);                             // Initialize bytes to zero
    data->type      = packet_type;                        // Signal that a data packet is being sent
    data->sqn       = 1;                                  // TODO Keep track of sequence numbers
    data->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp
    data->length = payload_size;                          // Update payload size
    memcpy((char*)data->_payload, payload, payload_size); // Copy payload to packet

    // Send packet
    if ( (bytes_sent = send(socket, data, packet_size, 0)) <= 0)
        throw std::runtime_error(appendErrorMessage("Unable to send message to server"));

    // Free memory used for packet
    free(data);

    // Return number of bytes sent
    return bytes_sent;
}

message_record* BaseSocket::composeMessage(std::string sender_name, std::string message_content, int message_type)
{
    // Calculate total size of the message record struct
    int record_size = sizeof(message_record) + (message_content.length() + 1);

    // TODO Save type

    // Create a record for the message
    message_record* msg = (message_record*)malloc(record_size); // Malloc memory
    bzero(msg, record_size);                                    // Initialize bytes to zero
    strcpy(msg->username,sender_name.c_str());                  // Copy sender name
    msg->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp
    msg->length = message_content.length() + 1;                 // Update message length
    strcpy((char*)msg->_message ,message_content.c_str());      // Copy message

    //Return a pointer to it
    return msg;
}