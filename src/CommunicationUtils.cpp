#include "CommunicationUtils.h"

std::string CommunicationUtils::appendErrorMessage(const std::string message)
{
    return message + " (" + std::string(strerror(errno)) + ")";
}

coordinator *CommunicationUtils::composeCoordinatorUpdate(std::map<int, int> translation)
{
    // Create the arrays of old and new sockets
    uint16_t *sockets = (uint16_t *)malloc(sizeof(uint16_t) * 2 * translation.size());
    bzero((void *)sockets, sizeof(uint16_t) * 2 * translation.size());

    // Fill array
    size_t offset = 0;
    for (auto i = translation.begin(); i != translation.end(); ++i, offset += 2)
    {
        sockets[offset] = i->first;
        sockets[offset + 1] = i->second;
    }

    // Create the coordinator packet message
    coordinator *coord = (coordinator *)malloc(sizeof(uint16_t) + (2 * sizeof(uint16_t) * translation.size()));
    coord->counter = (uint16_t)translation.size();
    if (coord->counter > 0)
        memcpy((char *)coord->_sockets, sockets, sizeof(uint16_t) * 2 * translation.size());

    return coord;
}

replica_update *CommunicationUtils::composeReplicaUpdate(int identifier, int port)
{
    // Create structure
    replica_update *update = (replica_update *)malloc(sizeof(replica_update));
    bzero((void *)update, sizeof(replica_update));

    // Fill data
    update->identifier = identifier;
    update->port = port;

    // Return created structure
    return update;
}

message_update *CommunicationUtils::composeMessageUpdate(message_record *message, std::string groupname, int socket)
{
    // Calculate message size
    int message_size = sizeof(message_record) + message->length;

    // Allocate memory for message update
    message_update *new_update = (message_update *)malloc(sizeof(message_update) + message_size);
    bzero((void *)new_update, sizeof(message_update) + message_size);

    // Fill data
    strcpy(new_update->groupname, groupname.c_str());
    new_update->socket = socket;
    new_update->length = message_size;
    memcpy((char *)new_update->_message, message, message_size);

    /*
    std::cout << "Composed a message update packet:" << std::endl;
    std::cout << "Groupname: " << new_update->groupname << std::endl;
    std::cout << "Socket: " << new_update->socket << std::endl;
    std::cout << "Length: " << new_update->length << std::endl;
    std::cout << "+ Message record: " << std::endl;
    std::cout << "| Username" << ((message_record *)new_update->_message)->username << std::endl;
    std::cout << "| Port: " << ((message_record *)new_update->_message)->port << std::endl;
    std::cout << "| Length: " << ((message_record *)new_update->_message)->length << std::endl;
    std::cout << "| Type: " << ((message_record *)new_update->_message)->type << std::endl;
    std::cout << "| Timestamp: " << ((message_record *)new_update->_message)->timestamp << std::endl;
    std::cout << "| Message: " << ((message_record *)new_update->_message)->_message << std::endl;
    */

    // Return created message update structure
    return new_update;
}

login_update *CommunicationUtils::composeLoginUpdate(char *login, std::string ip, int port, int socket)
{
    // Create front end
    login_update *new_front_end = (login_update *)malloc(sizeof(login_update) + sizeof(message_record) + ((message_record *)login)->length);
    bzero((void *)new_front_end, sizeof(login_update) + sizeof(message_record) + ((message_record *)login)->length);

    // Prepare data
    strcpy(new_front_end->ip, ip.c_str());
    new_front_end->port = port;
    new_front_end->socket = socket;
    new_front_end->length = sizeof(message_record) + ((message_record *)login)->length;
    memcpy((char *)new_front_end->_login, login, new_front_end->length);

    /*
    std::cout << "[composeLoginUpdate] login update packet composed:" << std::endl;
    std::cout << "Username: " << ((message_record *)(new_front_end->_login))->username << std::endl;
    std::cout << "Groupname: " << ((message_record *)(new_front_end->_login))->_message << std::endl;
    std::cout << "PORT: " << new_front_end->port << std::endl;
    std::cout << "IP: " << new_front_end->ip << std::endl;
    std::cout << "Length: " << new_front_end->length << std::endl;
    */

    // Return Front-End register
    return new_front_end;
}

packet *CommunicationUtils::composePacket(int packet_type, char *payload, int payload_size)
{
    // Calculate packet size
    int packet_size = sizeof(packet) + payload_size;

    // Prepare packet
    packet *data = (packet *)malloc(packet_size);                                             // Malloc memory
    bzero((void *)data, packet_size);                                                         // Initialize bytes to zero
    data->type = packet_type;                                                                 // Signal that a data packet is being sent
    data->sqn = 1;                                                                            // TODO Keep track of sequence numbers
    data->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp
    data->length = payload_size;                                                              // Update payload size
    memcpy((char *)data->_payload, payload, payload_size);                                    // Copy payload to packet

    // Return packet
    return data;
}

int CommunicationUtils::sendPacket(int socket, int packet_type, char *payload, int payload_size)
{
    // Calculate size of packet that will be sent
    int packet_size = sizeof(packet) + payload_size;
    int bytes_sent = -1; // Number of bytes actually sent to the client

    // Prepare packet
    packet *data = (packet *)malloc(packet_size);                                             // Malloc memory
    bzero((void *)data, packet_size);                                                         // Initialize bytes to zero
    data->type = packet_type;                                                                 // Signal that a data packet is being sent
    data->sqn = 1;                                                                            // TODO Keep track of sequence numbers
    data->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp
    data->length = payload_size;                                                              // Update payload size
    memcpy((char *)data->_payload, payload, payload_size);                                    // Copy payload to packet

    // Send packet
    if ((bytes_sent = send(socket, data, packet_size, 0)) <= 0)
    {
        //std::cerr << appendErrorMessage("Unable to send message to server") << std::endl;
    }

    // Free memory used for packet
    free(data);

    // Return number of bytes sent
    return bytes_sent;
}

message_record *CommunicationUtils::composeMessage(std::string sender_name, std::string message_content, int message_type, int port)
{
    // Calculate total size of the message record struct
    int record_size = sizeof(message_record) + (message_content.length() + 1);

    // Create a record for the message
    message_record *msg = (message_record *)malloc(record_size); // Malloc memory
    bzero((void *)msg, record_size);                             // Initialize bytes to zero
    strcpy(msg->username, sender_name.c_str());                  // Copy sender name
    msg->port = port;
    msg->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp
    msg->type = message_type;                                                                // Update message type
    msg->length = message_content.length() + 1;                                              // Update message length
    strcpy((char *)msg->_message, message_content.c_str());                                  // Copy message

    //Return a pointer to it
    return msg;
}

int CommunicationUtils::receivePacket(int socket, char *buffer, int buf_size)
{
    int total_bytes = 0;              // Total number of bytes read
    int read_bytes = 0;               // Number of bytes in current read
    int payload_bytes = 0;            // Bytes read from the payload
    int header_size = sizeof(packet); // Size of the packet header

    // While the header hasn't arrived entirely and the socket was not closed
    while (read_bytes >= 0 && total_bytes < header_size &&
           (read_bytes = recv(socket, buffer + total_bytes, header_size - total_bytes, 0)) > 0)
    {
        // If data has arrived, increase total
        if (read_bytes > 0)
            total_bytes += read_bytes;
        // If socket was closed, reset total
        else
            total_bytes = -1;
    }
    // If the entire header arrived
    if (total_bytes == header_size)
    {
        // Reset number of read bytes
        read_bytes = 0;

        // While the entire payload hasn't arrived
        while (read_bytes >= 0 && payload_bytes < ((packet *)buffer)->length &&
               (read_bytes = recv(socket, buffer + total_bytes, ((packet *)buffer)->length - payload_bytes, 0)) > 0)
        {
            // If data has arrived, increase totals
            if (read_bytes > 0)
            {
                payload_bytes += read_bytes;
                total_bytes += read_bytes;
            }
            // If the socket was closed, reset total
            else
            {
                read_bytes = -1;
            }
        }
    }

    // Return amount of bytes read/written
    return total_bytes;
}
