#ifndef CONSTANTS_H
#define CONSTANTS_H

// Server/Client related constants
#define NAME_REGEX   "[A-Za-z][A-Za-z0-9\\.]{3,19}" // Regex for validating user and group names
#define IP_REGEX     "[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}" // Regex for validating IP
#define HIST_PATH    "./hist/"  // Path to group history files
#define SERVER_PORT  6789    // Port the remote server listens at
#define MAX_SESSIONS 2       // Maximum number of active sessions per user
#define USER_TIMEOUT 60      // Time (in seconds) for user to be kept alive
#define PACKET_MAX   2048    // Maximum size (in bytes) for a packet
#define MESSAGE_MAX  256     // Maximum size (in bytes) for a user message

// Packet types
#define PAK_DATA 1          // Message packet
#define PAK_COMMAND 2       // Command packet
#define PAK_KEEP_ALIVE 3    // Keep-Alive packet
#define PAK_SERVER_MESSAGE 4  // A exclusive package sent by server

#endif
