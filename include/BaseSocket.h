#include <sys/socket.h>
#include <string>
#include <cstring>
#include <errno.h>
#include <chrono>

#include "data_types.h"
#include "constants.h"

class BaseSocket {
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
};