#ifndef CONSTANTS_H
#define CONSTANTS_H

// Server/Client related constants
#define NAME_REGEX      "[A-Za-z][A-Za-z0-9\\.]{3,19}" // Regex for validating user and group names
#define IP_REGEX        "([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})|localhost" // Regex for validating IP
#define PORT_REGEX      "[0-9]+"   // Regex for validating port number
#define HIST_PATH       "./hist/"  // Path to group history files
#define SERVER_PORT     6789    // Port the remote server listens at
#define MAX_SESSIONS    2       // Maximum number of active sessions per user
#define USER_TIMEOUT    60      // Time (in seconds) for user to be kept alive
#define REPLICA_TIMEOUT 5       // Time (in seconds) for replica to be kept alive
#define PACKET_MAX      2048    // Maximum size (in bytes) for a packet
#define MESSAGE_MAX     256     // Maximum size (in bytes) for a user message
#define SLEEP_TIME      1       // Time (in seconds) between keep-alive packets

// Packet types regarding chat messages
#define PAK_DATA              1 // Message packet
#define PAK_COMMAND           2 // Command packet
#define PAK_KEEP_ALIVE        3 // Keep-Alive packet
#define PAK_SERVER_MESSAGE    4 // Server message packet

// Packet types regarding replica updates
#define PAK_UPDATE_DISCONNECT 5 // Update packet for client disconnections
#define PAK_UPDATE_MSG        6 // Update packet for messages
#define PAK_UPDATE_LOGIN      7 // Update packet with login information
#define PAK_UPDATE_REPLICA    8 // Update packet with replica information for updating new replicas

// Packet types regarding election messages
#define PAK_ELECTION_START        9     // Indicates that a election has started
#define PAK_ELECTION_ANSWER       10    // Indicates a answer from a replica to a election that started
#define PAK_ELECTION_COORDINATOR  11    // Indicates that the sender replica is now the leader replica
#define ELECTION_TIMEOUT          1     // Time (in seconds) for a election coordinator leader answer timeout

// Packet types regarding replica connection
#define PAK_LINK             12 // Link packet, used for registering replicas to current master

// Message types
#define SERVER_MESSAGE 1 // Indicates a message sent by server (login or logout message)
#define USER_MESSAGE   2 // Indicates a message sent by a user
#define LOGIN_MESSAGE  3 // Login message containing group the user wants to log into

#endif
