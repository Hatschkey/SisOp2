#include "BaseSocket.h"

std::string BaseSocket::appendErrorMessage(const std::string message)
{
    return message + " (" + std::string(strerror(errno)) + ")";
}

int BaseSocket::sendPacket(int socket, int packet_type, char* payload, int payload_size)
{
    int packet_size = -1;  // Size of packet to be sent
    int bytes_sent = -1;   // Number of bytes sent to the client

    // Prepare packet
    packet* data = (packet*)malloc(sizeof(*data) + sizeof(char) * payload_size);
    data->type      = packet_type; // Signal that a data packet is being sent
    data->sqn       = 1; // TODO Keep track of the packet sequence numbers
    data->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp
    data->length = payload_size;    // Update payload size
    memcpy((char*)data->_payload, payload, payload_size); // Copy payload to packet

    // Calculate packet size
    packet_size = sizeof(*data) + (sizeof(char) * payload_size);

    // Send packet
    if ( (bytes_sent = send(socket, data, packet_size, 0)) <= 0)
        throw std::runtime_error(appendErrorMessage("Unable to send message to server"));

    // Free memory used for packet
    free(data);

    // Return number of bytes sent
    return bytes_sent;
}