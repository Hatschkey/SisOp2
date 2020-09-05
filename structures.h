#include <iostream>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstdio>
#include <string.h>
#include <unistd.h>
#include <regex>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctime>
#include <chrono>

/* Server / User related constants*/
#define NAME_REGEX "[A-Za-z][A-Za-z0-9\\.]{3,19}" // Regex for user and group names
#define SERVER_PORT 6789    // Port the server listens at
#define MAX_SESSIONS 2      // Maximum active sessions per user
#define USER_TIMEOUT 60     // Time for user to be kept alive
#define PACKET_MAX  2048    // Maximum size for a sent packet


/* Errors*/
#define ERROR_SOCKET_CREATION   -1
#define ERROR_IP_INVALID        -2
#define ERROR_CONNECTION_FAILED -3
#define ERROR_SOCKET_BIND       -4
#define ERROR_SOCKET_LISTEN     -5
#define ERROR_INVALID_USERNAME  -6
#define ERROR_INVALID_GROUPNAME -7
#define ERROR_THREAD_CREATION   -8
#define ERROR_SOCKET_OPTION     -9


/* Packet sent between server and client, containing messages or commands (Accessed by both client and server) */
#define PAK_DAT 0 // Message packet
#define PAK_CMD 1 // Command packet
#define PAK_KAL 2 // Keep-Alive packet
typedef struct __packet
{
    uint16_t type;          // Packet type (PAK_DAT || PAK_CMD)
    uint16_t sqn;           // Sequence number
    uint16_t length;        // Message lenght
    uint16_t timestamp;     // Message timestamp
    const char _payload[];        // Message payload

} packet;

/* Struct defining the login payload for clients trying to connect to the server */
typedef struct __login_payload
{
    char username[21];
    char groupname[21];

} login_payload;

/* Information about a user connected to the application (Managed by the server) */
typedef struct __user
{
    char username[20];          // User display name
    uint16_t active_sessions;   // Current active session count (Must be less than MAX_SESSIONS)
    uint16_t last_seen;         // Timestamp for last message received by this user
    
} user_t;

/* Information about a group (Managed by the server) */
typedef struct __group
{
    char groupname[20];
    
} group_t;

/* Shared data between the manager threads */
/* TODO Protect the entire struct with a mutex, or each single variable with it's mutex */
typedef struct __managed_data
{
    int stop_issued = 0;    // Signal if the server is stopping to threads
    
    std::vector<group_t>                   active_groups;  // Current active groups 
    std::vector<user_t>                    active_users;   // Current active users 
    std::map<group_t, std::vector<user_t>> sessions;       // Current users in each group 

} managed_data_t;

