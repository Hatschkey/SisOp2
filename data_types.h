#include <iostream>

// Packet that is sent between server and client
typedef struct __packet
{
    uint16_t type;          // Packet type 
    uint16_t sqn;           // Sequence number
    uint16_t length;        // Message lenght
    uint16_t timestamp;     // Message timestamp
    const char _payload[];  // Message payload

} packet;

typedef struct __login_payload
{
    char username[21];
    char groupname[21];

} login_payload;

typedef struct __message_payload
{
    /* TODO */

} message_payload;