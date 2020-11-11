/**
 * This file models the server-side Replica Manager which controls the chat logic by
 * handling requests from the Front End and manipulating the Group and User instances.
 * 
 * If this replica manager is the current master (i.e. elected leader) it should listen 
 * for any incoming client connections and front-ends requests, and propagate every change 
 * in the system state to the other replicas.
 * 
 * All the replicas communicate through a socket connection, and exchange messages for
 * keeping the system state consistent. When idle, the replicas exchange keep-alive 
 * messages every SLEEP_TIME in order to detect when a replica fails, which leads to
 * a new election. The elected replica becomes the new replica manager, and sends an 
 * 'elected' message to the remaining replicas. It also sends a message to each Front-End
 * that was previously registered (<ip-address>,<port>), in order for them to keep
 * connected to the server.
 * 
 * When the last replica is ended, it send a "disconnect" message to every fron end, informing
 * the clients that the server as a whole is begin shut-down.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <atomic>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#ifndef REPLICA_MANAGER_H
#define REPLICA_MANAGER_H

// Message exchange and mutual exclusion
#include "CommunicationUtils.h"
#include "RW_Monitor.h"

// Domain classes
#include "User.h"
#include "Group.h"
#include "Session.h"

// Constant values and data types
#include "constants.h"
#include "data_types.h"

class ReplicaManager : protected CommunicationUtils
{
private:
    // Server admin commands
    typedef void (*command_function)(void);
    static std::map<std::string, command_function> available_commands; // All available administrator commands
    static pthread_t command_handler_thread;                           // Thread for handling administrator input

    // Client connection logic
    static struct sockaddr_in server_address; // Replica manager socket address
    static struct sockaddr_in client_address; // New client socket address
    static int main_socket;                   // Socket where the replica manager listens for connections (new clients)

    static std::map<int, pthread_t> front_end_threads; // Socket descriptor and threads for handling front-end connections
    static RW_Monitor fe_threads_monitor;              // Monitor for front end trhead list

    static std::map<int, std::pair<std::string, int>> clients; // Map with client IPs and listening ports
    static RW_Monitor clients_monitor;                         // Monitor for the client IPs / Ports map

    // Replication logic

    static int ID;   // Unique ID for this replica
    static int port; // Listening port for this replica

    struct sockaddr_in replica_address; // New replica socket address

    static std::map<int, pthread_t> replica_manager_threads; // Socket descriptor and threads for handling replica manager connections
    static RW_Monitor rm_threads_monitor;                    // Monitor for replica manager thread list

    static pthread_t keep_alive_thread; // Keep alive thread

    // Election logic
    static int leader;                            // Current leader process
    static int leader_port;                       // Current leader port
    static std::string leader_ip;                 // Current leader's IP address
    static int leader_socket;                     // Socket where communication with the leader is going on
    static std::map<int, bool> replicas_answered; // Map with replica id and if the replica has answered an election or not
    static RW_Monitor ra_monitor;                 // Monitor for the replicas_answered map
    static std::atomic<bool> election_started;    // If there is a ongoing election

    static std::map<int, std::pair<int, int>> replicas; // Current replicas socket - id - listening-port map
    static RW_Monitor replicas_monitor;                 // Monitor for the replica list

    // Business logic
    static int message_history;                   // How many messages are sent to client upon login
    static std::map<int, Session *> session_list; // Map containing all the sessions currently active and their sockets
    static RW_Monitor session_monitor;            // Monitor for the session list

    // Other
    static std::atomic<bool> stop_issued;

public:
    // CONSTRUCTOR AND DESTRUCTOR

    /**
     * @brief Class Constructor
     * @param history      Amount of old messages to show client upon connection
     * @param port_        Port this replica listens at
     * @param id_          Unique identification number given to this replica
     * @param leader_ip_   Leader IP address
     * @param leader_port_ Port where the current leader is listening at
     * @param leader_      If this thread is the current leader
     */
    ReplicaManager(int history, int port_, int id_, std::string leader_ip_, int leader_port_, int leader_);

    /**
     * @brief Class Destructor 
     */
    ~ReplicaManager();

    // CONNECTION LOGIC

    /**
     * @brief Setus up the socket for connecting to the current leader 
     * @param new_replica_port  Port of the replica being connected to
     * @param new_replica_ip    Ip address of the replica being connected to
     * @param new_replica_id    Identifier of the replica being connected to
     * @returns Replica communication socket descriptor
     */
    static int setupReplicaConnection(int new_replica_port, std::string new_replica_ip, int new_replica_id);

    /**
     * @brief Setus up the socket for listening to connections 
     */
    static void setupLeaderConnection();

    /**
     * @brief Performs the setup needed for completing the leader connection 
     */
    static void *leaderCommunication(void *arg);

    /**
     * @brief Listens for incoming connections
     */
    static void *listenConnections(void *arg);

    /**
     * @brief Handles a newly created connection to distinguish between
     * front-end and replica connection, and calling the appropriate
     * handler for it
     */
    static void *handleUnkownConnection(void *arg);

    /**
     * @brief Handles communication with the front ends
     */
    static void handleFEConnection(int socket, packet *login);

    /**
     * @brief Handles communication with the other replica managers 
     */
    static void *handleRMConnection(void *arg);

    /**
     * @brief Sends messages to other replicas periodically, to detect server failure
     */
    static void *keepAlive(void *arg);

    // REPLICATION LOGIC

    /**
     * @brief Sends information about replica managers and front-ends that
     * are connected to the current master (ip and port, for example)
     * @param socket   Socket where the communication with the replica is currently happening
     * @param new_id   Indentifier of the new replica that is connecting
     * @param new_port Port where the new replica is listening at
     */
    static void catchUpReplica(int socket, int new_id, int new_port);

    /**
     * @brief Propagates a state update to all the other replicas.
     * OBS.: This should be used only by the current leader
     * @param update_payload Data being sent to the replicas
     * @param payload_size The size of the data being sent
     * @param type The type of update being sent
     */
    static void updateAllReplicas(void *update_payload, int payload_size, int type);

    // ELECTION LOGIC

    /**
     * @brief Start an election by sending PAK_ELECTION to every replica which id 
     * is greater than the sender replice id
     */
    static void startElection();

    /**
     * @brief Removes the last replica leader(who crashed)
     */
    static void removeReplicaLeader();

    // BUSINESS LOGIC

    /**
     * @brief Processes a login packet
     * @param login_info The login information received from the client
     * @param socket The socket descriptor for the socket were this login came from
     * @param master If this is being processed in a replica or on master (true = master, false = replica)
     * @returns An instance of Session, or NULL if could not create
     */
    static Session *processLogin(message_record *login_info, int socket, bool master);

    // ADMINISTRATOR COMMANDS

    /**
     * @brief Lists all commands available to administrator 
     */
    static void listCommands();

    /**
     * @brief Handles commands received from the replica administrator 
     */
    static void *handleCommands(void *arg);

    /**
     * @brief Issues a stop to this replica 
     */
    static void issueStop();

    /**
     * @brief Lists all threads running 
     */
    static void listThreads();

    /**
     * @brief Simulates a replica crash by stopping the keep-alive thread 
     */
    static void simulateCrash();

    /**
     * @brief Outputs current leader id
     */
    static void currentLeader();

    // ERROR HANDLING

    /**
     * @brief Handles a write operation to a closed socket
     * @param signal Received singal 
     */
    static void handleSIGPIPE(int signal);
};

#endif