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
#include <signal.h>
#include <poll.h>
#include <ctime>
#include <chrono>
#include "constants.h"

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
    char username[21];          // User display name
    uint16_t active_sessions;   // Current active session count (Must be less than MAX_SESSIONS)
    uint16_t last_seen;         // Timestamp for last message received by this user
    
} user_t;

/* Information about a group (Managed by the server) */
typedef struct __group
{
    char groupname[21];     // Group name
    FILE* history;          // Group chat history file handle
    int user_count;         // Current active user count in group

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

