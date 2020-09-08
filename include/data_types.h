#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <iostream>
#include <map>

// Packet that is sent between server and client
typedef struct __packet
{
    uint16_t type;          // Packet type 
    uint16_t sqn;           // Sequence number
    uint16_t length;        // Message lenght
    uint16_t timestamp;     // Message timestamp
    const char _payload[];  // Message payload

} packet;

// Payload for a client login packet
typedef struct __login_payload
{
    char username[21];  // Client username
    char groupname[21]; // Groupname

} login_payload;

/**
 * Payload for a client message
 * TODO Currently not used
 */
typedef struct __message_payload
{

} message_payload;

#endif