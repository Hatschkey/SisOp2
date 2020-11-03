#ifndef CONSTANTS_H
#define CONSTANTS_H

// Server/Client related constants
#define NAME_REGEX     "[A-Za-z][A-Za-z0-9\\.]{3,19}" // Regex for validating user and group names
#define IP_REGEX       "([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})|localhost" // Regex for validating IP
#define PORT_REGEX     "[0-9]+"   // Regex for validating port number
#define HIST_PATH      "./hist/"  // Path to group history files
#define SERVER_PORT    6789    // Port the remote server listens at
#define MAX_SESSIONS   2       // Maximum number of active sessions per user
#define USER_TIMEOUT   60      // Time (in seconds) for user to be kept alive
#define SERVER_TIMEOUT 5       // Time (in seconds) for replica to be kept alive
#define PACKET_MAX     2048    // Maximum size (in bytes) for a packet
#define MESSAGE_MAX    256     // Maximum size (in bytes) for a user message
#define SLEEP_TIME     1       // Time (in seconds) between keep-alive packets

// Packet types regarding chat messages
#define PAK_DATA              1 // Message packet
#define PAK_COMMAND           2 // Command packet
#define PAK_KEEP_ALIVE        3 // Keep-Alive packet
#define PAK_SERVER_MESSAGE    4 // Server message packet

// Packet types regarding election messages
#define PAK_ELECTION          5 // Election packet

// Packet types regarding replica updates
#define PAK_UPDATE_DISCONNECT 6 // Update packet for client disconnections
#define PAK_UPDATE_MSG        7 // Update packet for messages
#define PAK_UPDATE_LOGIN      8 // Update packet with login information
#define PAK_UPDATE_REPLICA    9 // Update packet with replica information for updating new replicas

// Packet types regarding replica connection
#define PAK_LINK             10 // Link packet, used for registering replicas to current master

// Message types
#define SERVER_MESSAGE 1 // Indicates a message sent by server (login or logout message)
#define USER_MESSAGE   2 // Indicates a message sent by a user
#define LOGIN_MESSAGE  3 // Login message containing group the user wants to log into

// Election message types
#define ELECTED_MESSAGE  1 // Message contains a request to start an election / a vote from the process
#define VOTE_MESSAGE     2 // Message contains process that has been elected as the new leader

#endif
