#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <iostream>
#include <map>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// PACKET

// Packet that is sent between server and client
typedef struct __packet
{
    uint16_t type;         // Packet type
    uint16_t sqn;          // Sequence number
    uint16_t length;       // Message lenght
    uint64_t timestamp;    // Message timestamp
    const char _payload[]; // Message payload

} packet;

// BUSINESS LOGIC

// Struct for recording a chat message, for storage in the history file
typedef struct __message_record
{
    char username[23];     // User who sent this message
    uint16_t length;       // Message length
    uint16_t type;         // Message's record type. Can be a SERVER_MESSAGE or USER_MESSAGE
    uint64_t timestamp;    // Message timestamp for ordering purposes
    const char _message[]; // What the user said

} message_record;

// REPLICA UPDATES

// Struct for updating new replicas with the current existing ones
typedef struct
{
    int identifier; // Replica unique ID
    int port;       // Replica listening port
                    // TODO Ip? not really needed

} replica_update;

// Struct for registering a new front end into the replica's list
typedef struct
{
    char ip[16];         // Front-end IP address
    uint16_t port;       // Front-end listening port
    uint16_t socket;     // Socket where this connection was established
    uint16_t length;     // Length of the login record
    const char _login[]; // Login message record

} login_update;

// Struct for updating message history between replicas
typedef struct
{
    char groupname[23];    // Group where this message is posted
    uint16_t socket;       // Socket where this message came from
    uint16_t length;       // Length of the actual message
    const char _message[]; // Actual message record

} message_update;

// LEADER ELECTION

// Struct for modeling messages sent between processes during the election
typedef struct __election_message
{
    uint16_t type; // Election message type
    uint16_t vote; // Process that was elected or vote from sender process, depending on type

} election_message;

// Struct that holds the new server address
typedef struct __new_server_addr
{
    in_addr_t ip;   // New server IP
    in_port_t port; // New server Port

} new_server_addr;


#endif
