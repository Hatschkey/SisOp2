// Server/Client related constants
#define NAME_REGEX   "[A-Za-z][A-Za-z0-9\\.]{3,19}" // Regex for validating user and group names
#define IP_REGEX     "[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}" // Regex for validating IP
#define SERVER_PORT  6789    // Port the remote server listens at
#define MAX_SESSIONS 2       // Maximum number of active sessions per user
#define USER_TIMEOUT 60      // Time (in seconds) for user to be kept alive
#define PACKET_MAX   2048    // Maximum size (in bytes) for a packet

// Error codes TODO FUCK THIS
#define ERROR_SOCKET_CREATION   -1
#define ERROR_IP_INVALID        -2
#define ERROR_CONNECTION_FAILED -3
#define ERROR_SOCKET_BIND       -4
#define ERROR_SOCKET_LISTEN     -5
#define ERROR_INVALID_USERNAME  -6
#define ERROR_INVALID_GROUPNAME -7
#define ERROR_THREAD_CREATION   -8
#define ERROR_SOCKET_OPTION     -9

// Packet types
#define PAK_DAT 0 // Message packet
#define PAK_CMD 1 // Command packet
#define PAK_KAL 2 // Keep-Alive packet