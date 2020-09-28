#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <iostream>
#include <map>
#include <ctime>

// Packet that is sent between server and client
typedef struct __packet
{
    uint16_t type;          // Packet type
    uint16_t sqn;           // Sequence number
    uint16_t length;        // Message lenght
    uint64_t timestamp;     // Message timestamp
    const char _payload[];  // Message payload

} packet;

// Struct for recording a chat message, for storage in the history file
typedef struct __message_record
{
    char username[23];      // User who sent this message (20 for name, 1 for \0 and 2 for brackets)
    uint16_t length;        // Message length
    uint16_t type;          // Message's record type. Can be a SERVER_MESSAGE or USER_MESSAGE
    uint64_t timestamp;     // Message timestamp for ordering purposes
    const char _message[];  // What the user said

} message_record;

#endif