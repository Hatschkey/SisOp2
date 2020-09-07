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

// Information about a user connected to the application
typedef struct __user
{
    char username[21];          // User display name
    uint16_t active_sessions;   // Current active session count (Must be less than MAX_SESSIONS)
    uint16_t last_seen;         // Timestamp for last message received from this user
    
} user_t;

// Information about a currently in-use group
typedef struct __group
{
    char groupname[21];     // Group name
    FILE* history;          // Group chat history file handle
    int user_count;         // Current active user count in group

} group_t;

/**
 * Shared data between the server threads
 * TODO Protect the entire struct with a mutex, or each single variable with it's mutex
 */
typedef struct __managed_data
{
    int stop_issued = 0;    // Signal if the server is stopping to threads

    std::map<std::string, group_t*>     active_groups;  // Current active groups 
    std::map<std::string, user_t*>      active_users;   // Current active users 

} managed_data_t;
