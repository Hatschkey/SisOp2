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

// Struct for recording a chat message, for storage in the history file
typedef struct __message_record
{
    char username[21];      // User who sent this message
    uint16_t timestamp;     // Message timestamp for ordering purposes
    uint16_t length;        // Message length
    const char _message[];  // What the user said

} message_record;

#endif