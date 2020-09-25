#ifndef COMMUNICATIONUTILS_H
#define COMMUNICATIONUTILS_H

#include <sys/socket.h>
#include <string>
#include <cstring>
#include <errno.h>
#include <chrono>

#include "data_types.h"
#include "constants.h"

class CommunicationUtils {
    
    protected:
    // Protected methods
    static std::string appendErrorMessage(const std::string message); // Add details of the error to the message

    /**
     * Sends a packet with given payload to the provided socket descriptor
     * @param socket Socket descriptor where the packet will be sent
     * @param packet_type Type of packet that should be sent (see constants.h)
     * @param payload Data that will be sent through that socket
     * @param payload_size Size of the data provided in payload
     * @returns Number of bytes sent
     */
    static int sendPacket(int socket, int packet_type, char* payload, int payload_size);

    /**
     * Creates a struct of type message_record with the provided data
     * @param sender_name Username of the user who sent this message
     * @param message_content Actual chat message
     * @param message_type Type of message TODO
     * @returns Pointer to the created message record
     */
    static message_record* composeMessage(std::string sender_name, std::string message_content, int message_type);


    /**
     * Tries to fully receive a packet from the informed socket, putting it in buffer
     * @param   socket From whence to receive the packet
     * @param   buffer Buffer where received data should be put
     * @param   buf_size Max size of the passed buffer
     * @returns Number of bytes received and put into buffer
     */
    static int receivePacket(int socket, char* buffer, int buf_size);
};

#endif