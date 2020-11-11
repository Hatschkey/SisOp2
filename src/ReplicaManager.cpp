#include "ReplicaManager.h"

// Server admin commands
typedef void (*command_function)(void);
std::map<std::string, command_function> ReplicaManager::available_commands = {

    {"help", &ReplicaManager::listCommands},
    {"stop", &ReplicaManager::issueStop},
    {"list groups", &Group::listGroups},
    {"list users", &User::listUsers},
    {"list threads", &ReplicaManager::listThreads},
    {"sim crash", &ReplicaManager::simulateCrash},
    {"list current leader", &ReplicaManager::currentLeader},

};
pthread_t ReplicaManager::command_handler_thread;

// Client connection logic
struct sockaddr_in ReplicaManager::server_address;
struct sockaddr_in ReplicaManager::client_address;
int ReplicaManager::main_socket;

std::map<int, pthread_t> ReplicaManager::front_end_threads;
RW_Monitor ReplicaManager::fe_threads_monitor;

std::map<int, std::pair<std::string, int>> ReplicaManager::clients;
RW_Monitor ReplicaManager::clients_monitor;

// Replication logic
int ReplicaManager::ID;
int ReplicaManager::port;
std::map<int, pthread_t> ReplicaManager::replica_manager_threads;
RW_Monitor ReplicaManager::rm_threads_monitor;
pthread_t ReplicaManager::keep_alive_thread;

// Election logic
int ReplicaManager::leader;
int ReplicaManager::leader_port;
std::string ReplicaManager::leader_ip;
int ReplicaManager::leader_socket;

std::map<int, bool> ReplicaManager::replicas_answered;
RW_Monitor ReplicaManager::ra_monitor;

std::atomic<bool> ReplicaManager::election_started;

std::map<int, std::pair<int, int>> ReplicaManager::replicas;
RW_Monitor ReplicaManager::replicas_monitor;

// Business logic
int ReplicaManager::message_history;
std::map<int, Session *> ReplicaManager::session_list;
RW_Monitor ReplicaManager::session_monitor;

// Other
std::atomic<bool> ReplicaManager::stop_issued;

ReplicaManager::ReplicaManager(int history, int port_, int id_, std::string leader_ip_, int leader_port_, int leader_)
{
    this->message_history = history;
    this->leader = leader;
    this->ID = id_;
    this->port = port_;
    this->leader_ip = leader_ip_;
    this->leader_port = leader_port_;

    // If this is not the leader replica
    if (this->leader != this->ID)
    {
        // Setup the new connection with leader
        ReplicaManager::leader_socket = ReplicaManager::setupReplicaConnection(leader_port_, leader_ip_, leader_);

        // Spawn thread to communicate with leader, and add it to the list
        pthread_t leader_communication;
        pthread_create(&leader_communication, NULL, leaderCommunication, NULL);

        replica_manager_threads.insert(std::make_pair(leader_socket, leader_communication));
    }

    // Setup connection
    ReplicaManager::setupLeaderConnection();

    // Spawn thread for listening to administrator commands
    pthread_create(&command_handler_thread, NULL, handleCommands, NULL);

    // Spawn thread for keeping replica alive
    pthread_create(&keep_alive_thread, NULL, keepAlive, NULL);

    // Register error signal handler
    signal(SIGPIPE, ReplicaManager::handleSIGPIPE);
}

ReplicaManager::~ReplicaManager()
{
    // Close client listener socket
    close(main_socket);

    // TODO Anything else?
}

int ReplicaManager::setupReplicaConnection(int new_replica_port, std::string new_replica_ip, int new_replica_id)
{
    int new_socket = -1;

    // Create socket
    if ((new_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error(appendErrorMessage("Error during socket creation"));

    // Fill server socket address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(new_replica_port);
    server_address.sin_addr.s_addr = inet_addr(new_replica_ip.c_str());

    // Try to connect to replica
    if (connect(new_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        throw std::runtime_error(appendErrorMessage("Error connecting to server"));

    // Request write rights
    replicas_monitor.requestWrite();

    // Add to replica manager list
    replicas.insert(std::make_pair(new_socket, std::make_pair(new_replica_id, new_replica_port)));

    // Release write rights
    replicas_monitor.releaseWrite();

    return new_socket;
}

void ReplicaManager::setupLeaderConnection()
{
    // Create socket
    if ((main_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error(appendErrorMessage("Error during socket creation"));

    // Prepare server socket address (Where the server listens at)
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(ReplicaManager::port);

    // Set socket options
    int yes = 1;
    if (setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        throw std::runtime_error(appendErrorMessage("Error setting socket options"));

    // Bind socket
    if (bind(main_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        throw std::runtime_error(appendErrorMessage("Error during socket bind"));
}

void *ReplicaManager::leaderCommunication(void *arg)
{
    replica_update *link_message = NULL;

    // Compose link message
    link_message = CommunicationUtils::composeReplicaUpdate(ReplicaManager::ID, ReplicaManager::port);

    // Send link message to current leader
    CommunicationUtils::sendPacket(ReplicaManager::leader_socket, PAK_LINK, (char *)link_message, sizeof(replica_update));

    // Free data structure
    free(link_message);

    // Handle connection with replica manager passing it's socket as argument
    handleRMConnection((void *)&ReplicaManager::leader_socket);

    return NULL;
}

void *ReplicaManager::listenConnections(void *arg)
{
    int *new_socket = NULL;
    size_t sockaddr_size = sizeof(struct sockaddr_in);

    int socket = -1; // New connection socket

    // Set passive listen socket
    if (listen(main_socket, 3) < 0)
        throw std::runtime_error(appendErrorMessage("Error setting socket as passive listener"));

    // Output ready info
    std::cout << "Replica " << ReplicaManager::ID << " ready to receive new connections" << std::endl;
    std::cout << "Current leader is " << ReplicaManager::leader << std::endl;
    ReplicaManager::listCommands();

    // Wait for new connections
    while (!stop_issued && (socket = accept(main_socket, (struct sockaddr *)&client_address, (socklen_t *)&sockaddr_size)) > 0)
    {
        pthread_t new_thread;

        // Get a reference to the socket
        new_socket = (int *)malloc(sizeof(int));
        *new_socket = socket;

        // Spawn new thread for handling that connection
        if (pthread_create(&new_thread, NULL, handleUnkownConnection, (void *)new_socket) < 0)
        {
            // Close socket if no thread was created
            std::cerr << "Could not create thread for socket " << socket << std::endl;
            close(socket);
        }
    }

    // Issue a stop command
    ReplicaManager::issueStop();

    // Request write rights
    fe_threads_monitor.requestWrite();

    // Wait for all front end threads to finish
    for (std::map<int, pthread_t>::iterator i = front_end_threads.begin(); i != front_end_threads.end(); ++i)
    {
        std::cout << "Waiting for client communication to end on socket " << i->first << "..." << std::endl;

        // Join thread
        pthread_join(i->second, NULL);
    }

    // Clear map
    front_end_threads.clear();

    // Release write rights
    fe_threads_monitor.releaseWrite();

    // Request write rights
    rm_threads_monitor.requestWrite();

    // Wait for all replica manager threads to finish
    for (auto i = replica_manager_threads.begin(); i != replica_manager_threads.end(); ++i)
    {
        std::cout << "Waiting for replica communication to end on socket " << i->first << "..." << std::endl;

        // Join thread
        pthread_join(i->second, NULL);
    }

    // Clear map
    replica_manager_threads.clear();

    // Release write rights
    rm_threads_monitor.releaseWrite();

    std::cout << "Waiting for command handler to end..." << std::endl;

    // Join with the command handler thread
    pthread_join(command_handler_thread, NULL);

    // Join with the keep-alive thread
    pthread_cancel(ReplicaManager::keep_alive_thread);
    pthread_join(keep_alive_thread, NULL);

    return NULL;
}

void *ReplicaManager::handleUnkownConnection(void *arg)
{
    int socket = *(int *)arg;           // Socket assigned to connection
    struct timeval timeout;             // Timeout struct
    char buffer[PACKET_MAX];            // Buffer for message
    int read_bytes = -1;                // Number of bytes read from socket
    packet *received_packet = NULL;     // Received message as a packet structure
    replica_update *new_replica = NULL; // New replica communication info

    pthread_t self = pthread_self(); // Get current thread id

    // Clear buffer
    bzero((void *)buffer, PACKET_MAX);

    // Receive first message from connection (Identification as client or replica)
    read_bytes = recv(socket, buffer, PACKET_MAX, 0);

    // If message was received ok
    if (read_bytes > 0)
    {
        // Decode message into a packet format
        received_packet = (packet *)buffer;

        std::cout << "Received packet of type " << received_packet->type << " from the new connection on socket " << socket << std::endl;

        switch (received_packet->type)
        {
        case PAK_COMMAND: // Login packet, came from a client

            // Request write rights
            fe_threads_monitor.requestWrite();

            // Add thread to list of front end handlers
            front_end_threads.insert(std::make_pair(socket, self));

            // Release write rights
            fe_threads_monitor.releaseWrite();

            // Set the keep-alive timer on the socket
            timeout = {.tv_sec = USER_TIMEOUT, .tv_usec = 0};
            if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
                throw std::runtime_error(appendErrorMessage("Error setting socket options"));

            // Start listening for next messages
            ReplicaManager::handleFEConnection(socket, received_packet);

            break;
        case PAK_LINK: // Link packet, came from a replica

            // Decode update
            new_replica = (replica_update *)(received_packet->_payload);

            // Request write rights
            rm_threads_monitor.requestWrite();

            // Add thread to list of replica manager handlers
            replica_manager_threads.insert(std::make_pair(socket, self));

            // Release write rights
            rm_threads_monitor.releaseWrite();

            // Request write rights
            replicas_monitor.requestWrite();

            // Update replica list
            replicas.insert(std::make_pair(socket, std::make_pair(new_replica->identifier, new_replica->port)));

            // Release write rights
            replicas_monitor.releaseWrite();

            if (new_replica->identifier > ReplicaManager::ID)
            {
                // Request write rights
                ra_monitor.requestWrite();

                // Add to the replicas_answered map
                replicas_answered.insert(std::make_pair(new_replica->identifier, false));

                // Release write rights
                ra_monitor.releaseWrite();
            }

            // Set the keep-alive timer on the socket
            timeout = {.tv_sec = REPLICA_TIMEOUT, .tv_usec = 0};
            if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
                throw std::runtime_error(appendErrorMessage("Error setting socket options"));

            // If this is the leader
            if (ReplicaManager::ID == ReplicaManager::leader)
            {
                // Send information about connected front-ends and replicas
                ReplicaManager::catchUpReplica(socket, new_replica->identifier, new_replica->port);
            }

            // Start listening for next messages
            ReplicaManager::handleRMConnection((void *)&socket);

            break;
        default: // Anything else does not make sense
            std::cerr << "Invalid packet type (" << received_packet->type << ") received from socket " << socket << std::endl;
            break;
        }
    }

    // Free received argument
    free(arg);

    // Exit
    pthread_exit(NULL);
}

void ReplicaManager::handleFEConnection(int socket, packet *login)
{
    int read_bytes = -1;     // Number of bytes read from socket
    char buffer[PACKET_MAX]; // Buffer for message

    packet *received_packet = NULL;  // Received message as a packet structure
    message_record *message = NULL;  // Received packet content
    login_update *front_end = NULL;  // Composed front-end information for updating the replicas
    message_update *update = NULL;   // Message update structure for sending to replicas
    Session *current_session = NULL; // Session info for this client

    // Get client IP and port
    std::string front_end_ip = inet_ntoa(client_address.sin_addr);
    int front_end_port = client_address.sin_port; // TODO Wrong port i think

    // Create front end registry
    front_end = CommunicationUtils::composeLoginUpdate((char *)login->_payload, front_end_ip, front_end_port, socket);

    // Update replicas
    ReplicaManager::updateAllReplicas((void *)front_end, sizeof(login_update) + front_end->length, PAK_UPDATE_LOGIN);

    // Free front end registry
    free(front_end);

    // Process login
    if (!(current_session = ReplicaManager::processLogin((message_record *)login->_payload, socket, true)))
    {
        // Request write rights
        fe_threads_monitor.requestWrite();

        // Remove itself from the threads list
        front_end_threads.erase(socket);

        // Release write rights
        fe_threads_monitor.releaseWrite();

        // Return
        return;
    }

    // Request write rights
    clients_monitor.requestWrite();

    // Add to list of front ends
    ReplicaManager::clients.insert(std::make_pair(socket, std::make_pair(front_end_ip, front_end_port)));

    // Release write rights
    clients_monitor.releaseWrite();

    // Wait for messages
    while (!stop_issued && (read_bytes = recv(socket, buffer, PACKET_MAX, 0)) > 0)
    {
        // Decode received message into a packet structure
        received_packet = (packet *)buffer;

        if (received_packet->type != PAK_KEEP_ALIVE)
            std::cout << "Received packet of type " << received_packet->type << " from a front-end" << std::endl;

        // Based on packet type
        switch (received_packet->type)
        {
        case PAK_DATA:

            // Decode received message into a message record
            message = (message_record *)received_packet->_payload;

            // Compose a message update
            update = CommunicationUtils::composeMessageUpdate(message, current_session->getGroup()->groupname, socket);

            // Update replicas
            ReplicaManager::updateAllReplicas((void *)update, sizeof(message_update) + update->length, PAK_UPDATE_MSG);

            // Free message update structure
            free(update);
            update = NULL;

            // Send message
            if (current_session != NULL)
                current_session->messageGroup(message);

            break;
        case PAK_KEEP_ALIVE:
            // Do nothing
            break;
        default:
            std::cerr << "Unknown packet type (" << received_packet->type << ") received from front-end socket " << socket << std::endl;
            break;
        }

        // Reset buffer
        bzero((void *)buffer, PACKET_MAX);
    }

    // Update replicas
    ReplicaManager::updateAllReplicas((void *)&socket, sizeof(int), PAK_UPDATE_DISCONNECT);

    // Request write rights
    ReplicaManager::session_monitor.requestWrite();

    // Remove session from list
    ReplicaManager::session_list.erase(current_session->getId());

    // Release write rights
    ReplicaManager::session_monitor.releaseWrite();

    // Delete session
    delete current_session;

    // Check if connection ended due to timeout
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        // If so, close the connection for good
        close(socket);
    }

    // Remove itself from the FE threads list
    if (!stop_issued)
    {
        // Request write rights
        fe_threads_monitor.requestWrite();

        // Remove itself from the threads list
        front_end_threads.erase(socket);

        // Release write rights
        fe_threads_monitor.releaseWrite();
    }

    return;
}

void *ReplicaManager::handleRMConnection(void *arg)
{
    int socket = *(int *)arg;

    int read_bytes = -1;     // Number of bytes read from socket
    char buffer[PACKET_MAX]; // Buffer for message
    int client_socket = -1;

    packet *received_packet = NULL; // Received message as a packet structure
    message_record *message = NULL; // Received packet content
    login_update *fe_info = NULL;   // Front end registry structure
    message_update *update = NULL;  // Structure for incoming group message updates
    Session *new_session = NULL;    // Client sessions

    replica_update *new_rm = NULL; // New replica manager data
    int rm_socket = -1;            // New replica manager's assigned socket

    pthread_t rm_thread; // Spawned thread for handling new replicas

    // References to user and group
    Group *group = NULL;

    // To end a empty packet
    char empty = '\0';

    // Wait for messages
    /*read_bytes = recv(socket, buffer, PACKET_MAX, 0)) > 0*/
    while (!stop_issued && (read_bytes = CommunicationUtils::receivePacket(socket, buffer, PACKET_MAX)) > 0)
    {
        // Decode received message into a packet structure
        received_packet = (packet *)buffer;

        if (received_packet->type != PAK_KEEP_ALIVE)
            std::cout << "Received packet of type " << received_packet->type << " from replica connection" << std::endl;

        // Based on packet type
        switch (received_packet->type)
        {
        case PAK_UPDATE_LOGIN: // Client connected

            // Decode payload into a front end registry structure
            fe_info = (login_update *)(received_packet->_payload);

            // Process the client login
            if ((new_session = ReplicaManager::processLogin((message_record *)(fe_info->_login), fe_info->socket, false)) != NULL)
            {
                // Request write rights
                session_monitor.requestWrite();

                // Add to session list
                session_list.insert(std::make_pair(new_session->getId(), new_session));

                // Release write rights
                session_monitor.releaseWrite();

                // Request write rights
                clients_monitor.requestWrite();

                // Add the new front-end to this replica's list
                clients.insert(std::make_pair(fe_info->socket, std::make_pair(fe_info->ip, fe_info->port)));

                // Release write rights
                clients_monitor.releaseWrite();
            }

            break;
        case PAK_UPDATE_MSG: // Client sent a message

            // Decode payload into message update and it's payload into a message record
            update = (message_update *)received_packet->_payload;
            message = (message_record *)update->_message;

            // Get referenced group
            group = Group::getGroup(update->groupname);

            // Update it's history file
            group->saveMessage(message->_message, message->username, message->type);

            break;
        case PAK_UPDATE_DISCONNECT: // Client disconnected

            // Get corresponding disconnect socket from received packet payload
            client_socket = *(int *)(received_packet->_payload);

            // Request write rights
            session_monitor.requestWrite();

            // Delete session
            delete session_list.at(client_socket);

            // Remove session from list
            session_list.erase(client_socket);

            // Release write rights
            session_monitor.releaseWrite();

            // Request write rights
            clients_monitor.requestWrite();

            // Remove front-end from list
            clients.erase(client_socket);

            // Releasre write rights
            clients_monitor.releaseWrite();

            // Reset var
            client_socket = -1;

            break;
        case PAK_UPDATE_REPLICA: // A new replica connected

            // Decode structure into a replica update packet
            new_rm = (replica_update *)(received_packet->_payload);

            // Setup the connection to this new replica
            rm_socket = ReplicaManager::setupReplicaConnection(new_rm->port, "127.0.0.1", new_rm->identifier);

            // Spawn a new thread to handle that connection
            if (pthread_create(&rm_thread, NULL, handleRMConnection, (void *)&rm_socket) < 0)
            {
                // Close socket if no thread was created
                std::cerr << "Could not create thread for new replica manager (" << new_rm->identifier << ") at socket " << socket << std::endl;
                close(rm_socket);
            }

            // Send a link packet to the new replica manager
            new_rm = CommunicationUtils::composeReplicaUpdate(ReplicaManager::ID, ReplicaManager::port);

            // Send greetings to new replica
            CommunicationUtils::sendPacket(rm_socket, PAK_LINK, (char *)new_rm, sizeof(replica_update));

            // Free data structure
            free(new_rm);

            // Request write rights
            rm_threads_monitor.requestWrite();

            // Add thread to list of replica manager communication threads
            replica_manager_threads.insert(std::make_pair(rm_socket, rm_thread));

            // Release write rights
            rm_threads_monitor.releaseWrite();

            break;
        case PAK_ELECTION_START: // An election is happening

            // Answer the current election start message
            CommunicationUtils::sendPacket(socket, PAK_ELECTION_ANSWER, &empty, sizeof(empty));

            if (!election_started)
            {
                election_started = true;

                // Start his own election
                ReplicaManager::startElection();
}

            break;
        case PAK_ELECTION_ANSWER:

            std::cout << "Received answer packet, marking thread as answered" << std::endl;

            // Request write rights
            ra_monitor.requestWrite();

            // Add to the replicas_answered map
            replicas_answered[replicas[socket].first] = true;

            // Release write rights
            ra_monitor.releaseWrite();

            break;
        case PAK_ELECTION_COORDINATOR:

            std::cout << "Received coordinator packet, updating my leader information" << std::endl;

            // Update leader informations
            leader = replicas[socket].first;
            leader_port = replicas[socket].second;
            leader_socket = socket;

            break;
        case PAK_KEEP_ALIVE: // Keep-Alive
            // Do nothing
            break;
        default:
            break;
        }

        // Reset buffer
        bzero((void *)buffer, PACKET_MAX);
    }

    replicas_monitor.requestRead();

    int buddy_replica_id = -1;

    try
    {
        // Get the id of the replica whose thread was communicating with the rm who crashed
        buddy_replica_id = replicas.at(socket).first;
    }
    catch (const std::out_of_range &e)
    {
        // Leader was removed in a prior election
    }

    replicas_monitor.releaseRead();

    // If this was leader that timed out
    if (ReplicaManager::leader == buddy_replica_id && !election_started && !stop_issued)
    {
        std::cout << "Current leader (Replica " << buddy_replica_id << ") has stopped, starting election..." << std::endl;

        ReplicaManager::startElection();
    }

    // Check if connection ended due to timeout
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        std::cout << "Replica at socket " << socket << " timed out" << std::endl;

        // Close socket
        close(socket);
    }
    else
    {
        // Remove itself from the RM threads list
        if (!stop_issued)
        {
            // Request write rights
            rm_threads_monitor.requestWrite();

            // Remove itself from the threads list
            replica_manager_threads.erase(socket);

            // Release write rights
            rm_threads_monitor.releaseWrite();
        }
    }

    pthread_exit(NULL);
}

void *ReplicaManager::keepAlive(void *arg)
{
    char keep_alive = '\0';

    // For the entire time this replica is running
    while (!ReplicaManager::stop_issued)
    {

        // Sleep for SLEEP_TIME seconds
        sleep(SLEEP_TIME);

        // Request read rights
        rm_threads_monitor.requestRead();

        // For each connected replica
        for (auto i = ReplicaManager::replica_manager_threads.begin(); i != ReplicaManager::replica_manager_threads.end(); ++i)
        {
            // Send KAL packet
            if (i->first != ReplicaManager::ID)
                CommunicationUtils::sendPacket(i->first, PAK_KEEP_ALIVE, &keep_alive, sizeof(keep_alive));
        }

        // Release read rights
        rm_threads_monitor.releaseRead();
    }

    // Exit
    pthread_exit(NULL);
}

// REPLICATION UPDATES LOGIC

void ReplicaManager::catchUpReplica(int socket, int new_id, int new_port)
{
    replica_update *update = NULL;
    replica_update *new_replica = NULL;
    message_record *login_data = NULL;
    login_update *front_end_data = NULL;
    Session *session = NULL;

    // Request read rights
    replicas_monitor.requestRead();

    // Create a replica update structure for the new replica
    new_replica = CommunicationUtils::composeReplicaUpdate(new_id, new_port);

    // Iterate list of replica managers
    for (auto i = ReplicaManager::replicas.begin(); i != ReplicaManager::replicas.end(); ++i)
    {
        if ((i->second).first != new_id)
        {
            // Create a replica update structure for the existing replica
            update = CommunicationUtils::composeReplicaUpdate((i->second).first, (i->second).second);

            // Send to new replica
            CommunicationUtils::sendPacket(socket, PAK_UPDATE_REPLICA, (char *)update, sizeof(replica_update));

            // Free data structure
            free(update);
            update = NULL;
        }
    }

    // Free new replica update data structure
    free(new_replica);
    new_replica = NULL;

    // Release read rights
    replicas_monitor.releaseRead();

    // Request read rights
    clients_monitor.requestRead();

    // Iterate list of connected front-ends
    for (auto i = ReplicaManager::clients.begin(); i != clients.end(); ++i)
    {
        // Request read rights
        session_monitor.requestRead();

        // Get reference to session of this front-end
        session = session_list.at(i->first);

        // Release read rights
        session_monitor.releaseRead();

        // Compose a login message record
        login_data = CommunicationUtils::composeMessage(session->getUser()->username, session->getGroup()->groupname, PAK_COMMAND);

        // Composed a login update
        front_end_data = CommunicationUtils::composeLoginUpdate((char *)login_data, (i->second).first, (i->second).second, i->first);

        // Send to new replica
        CommunicationUtils::sendPacket(socket, PAK_UPDATE_LOGIN, (char *)front_end_data, sizeof(login_update) + front_end_data->length);

        // Free composed data structures
        free(login_data);
        free(front_end_data);
        login_data = NULL;
        front_end_data = NULL;
    }

    // Release read rights
    clients_monitor.releaseRead();
}

void ReplicaManager::updateAllReplicas(void *update_payload, int payload_size, int type)
{
    // Request read rights
    rm_threads_monitor.requestRead();

    // For each connected replica
    for (auto i = ReplicaManager::replica_manager_threads.begin(); i != ReplicaManager::replica_manager_threads.end(); ++i)
    {
        // Send update packet
        if (i->first != ReplicaManager::ID)
            CommunicationUtils::sendPacket(i->first, type, (char *)update_payload, payload_size);
    }

    // Release read rights
    rm_threads_monitor.releaseRead();
}

void ReplicaManager::startElection()
{
    removeReplicaLeader();

    // Request read rights
    replicas_monitor.requestRead();
    ra_monitor.requestWrite();

    int count = 0;
    bool someone_answered = false;

    char empty = '\0';

    // replicas_answered
    for (auto i = ReplicaManager::replicas.begin(); i != ReplicaManager::replicas.end(); ++i)
    {
        // If the current replica id is greater than the sender replica id
        if (i->second.first > ReplicaManager::ID)
        {
            CommunicationUtils::sendPacket(i->first, PAK_ELECTION_START, &empty, sizeof(empty));
            replicas_answered[i->second.first] = false;
            count++;
        }
    }

    // Release read rights
    replicas_monitor.releaseRead();
    ra_monitor.releaseWrite();

    // If this replica didn't send a packet to anyone, this indicates that he is now the leader
    if (count == 0)
    {
        // Send a coordinator packet to every replica
        ReplicaManager::updateAllReplicas(&empty, sizeof(empty), PAK_ELECTION_COORDINATOR);

        // New leader informations
        leader = ReplicaManager::ID;
        leader_port = ReplicaManager::port;
        leader_socket = ReplicaManager::main_socket;
    }
    else
    {
        // Saves the last leader id
        int old_leader_id = ReplicaManager::leader;

        sleep(ELECTION_TIMEOUT);

        if (old_leader_id == ReplicaManager::leader)
        {
            ra_monitor.requestRead();

            for (auto i = ReplicaManager::replicas_answered.begin(); i != ReplicaManager::replicas_answered.end(); ++i)
            {
                if (i->second)
                {
                    someone_answered = true;
                }
            }

            ra_monitor.releaseRead();

            // If no one has answered, this replica is the new leader
            if (!someone_answered)
            {
                // Send a coordinator packet to every replica
                ReplicaManager::updateAllReplicas(&empty, sizeof(empty), PAK_ELECTION_COORDINATOR);

                // New leader informations
                leader = ReplicaManager::ID;
                leader_port = ReplicaManager::port;
                leader_socket = ReplicaManager::main_socket;
            }
            else
            {
                // Sleeps a time waiting for a sign indicanting that there's a new leader
                sleep(ELECTION_TIMEOUT);

                // If the old leader id equals the actual leader id
                if (old_leader_id == ReplicaManager::leader)
                {
                    // Means that we didn't received a PAK_COORDINATOR and there isn't a new leader
                    // So we start a election again
                    startElection();
                }
            }
        }
    }
}

// BUSINESS LOGIC
Session *ReplicaManager::processLogin(message_record *login_info, int socket, bool master)
{
    // Create the session
    Session *new_session = new Session(login_info->username, login_info->_message, socket);

    // If session creation went ok
    if (new_session->isOpen())
    {
        // Request write rights
        ReplicaManager::session_monitor.requestWrite();

        // Add session to list
        ReplicaManager::session_list.insert(std::make_pair(new_session->getId(), new_session));

        // Release write rights
        ReplicaManager::session_monitor.releaseWrite();

        // If this is being processed on current master
        if (master)
        {
            // Send history to client
            new_session->sendHistory(ReplicaManager::message_history);
        }
    }
    else
    {
        // Delete the session
        delete new_session;

        // Set it to null
        new_session = NULL;
    }

    // Return result
    return new_session;
}

// ADMINISTRATOR COMMANDS

void ReplicaManager::listCommands()
{
    // Iterate map listing commands
    std::cout << "Available commands are: " << std::endl;
    for (std::map<std::string, command_function>::iterator i = ReplicaManager::available_commands.begin(); i != ReplicaManager::available_commands.end(); ++i)
    {
        std::cout << " " << i->first << std::endl;
    }
}

void *ReplicaManager::handleCommands(void *arg)
{
    // Get administrator commands until Ctrl D is pressed or 'stop' is typed
    std::string command;
    while (!stop_issued && std::getline(std::cin, command))
    {
        try
        {
            // Execute administrator command
            available_commands.at(command)();
        }
        catch (std::out_of_range &e)
        {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }

    // Signal all other threads to end
    if (!stop_issued)
        ReplicaManager::issueStop();

    // Exit
    pthread_exit(NULL);
}

void ReplicaManager::issueStop()
{
    ReplicaManager::stop_issued = true;

    // Request read rights
    ReplicaManager::fe_threads_monitor.requestRead();

    // Stop all communication with clients
    for (std::map<int, pthread_t>::iterator i = front_end_threads.begin(); i != front_end_threads.end(); ++i)
    {
        shutdown(i->first, SHUT_RDWR);
    }

    // Release read rights
    ReplicaManager::fe_threads_monitor.releaseRead();

    // Request read rights
    ReplicaManager::rm_threads_monitor.requestRead();

    // Stop all communication with replicas
    for (std::map<int, pthread_t>::iterator i = replica_manager_threads.begin(); i != replica_manager_threads.end(); ++i)
    {
        shutdown(i->first, SHUT_RDWR);
    }

    // Release read rights
    ReplicaManager::rm_threads_monitor.releaseRead();

    // Stop the main socket from receiving connections
    shutdown(ReplicaManager::main_socket, SHUT_RDWR);
}

void ReplicaManager::listThreads()
{
    // Request read rights
    fe_threads_monitor.requestRead();

    // Delimiter
    std::cout << "FRONT END THREADS" << std::endl;
    std::cout << "======================" << std::endl;

    // Iterate through threads
    for (std::map<int, pthread_t>::iterator i = front_end_threads.begin(); i != front_end_threads.end(); ++i)
    {
        std::cout << " FE Thread associated with socket " << i->first << std::endl;
    }
    // Delimiter
    std::cout << "======================" << std::endl;

    // Release read rights
    fe_threads_monitor.releaseRead();

    // Request read rights
    rm_threads_monitor.requestRead();
    replicas_monitor.requestRead();

    // Delimiter
    std::cout << "REPLICA MANAGER THREADS" << std::endl;
    std::cout << "======================" << std::endl;

    // Iterate through threads
    for (std::map<int, pthread_t>::iterator i = replica_manager_threads.begin(); i != replica_manager_threads.end(); ++i)
    {
        std::cout << " RM Thread associated with socket " << i->first << std::endl;
        std::cout << " + This thread is talking to a replica with ID " << replicas.at(i->first).first << std::endl;
    }
    // Delimiter
    std::cout << "======================" << std::endl;

    // Release read rights
    rm_threads_monitor.releaseRead();
    replicas_monitor.releaseRead();
}

void ReplicaManager::simulateCrash()
{
    std::cout << "Stopping keep-alive thread..." << std::endl;

    // Shutdown keep-alive thread
    pthread_cancel(ReplicaManager::keep_alive_thread);
    pthread_join(keep_alive_thread, NULL);

    // Sleep for 2*timeout seconds so the other replicas can notice and complete election
    sleep(2 * REPLICA_TIMEOUT);

    // End
    ReplicaManager::issueStop();
}

void ReplicaManager::removeReplicaLeader()
{
    replicas_monitor.requestWrite();
    ra_monitor.requestWrite();

    try
    {
        replicas.erase(leader_socket);
        replicas_answered.erase(ReplicaManager::leader);
    }
    catch (const std::out_of_range &e)
    {
        // Means that the leader was already removed
    }

    replicas_monitor.releaseWrite();
    ra_monitor.releaseWrite();
}

void ReplicaManager::currentLeader()
{
    std::cout << "Current leader replica is: " << ReplicaManager::leader << std::endl;
}

// ERROR SIGNAL HANDLERS

void ReplicaManager::handleSIGPIPE(int signal)
{
    //std::cerr << "Error sending packet " << std::endl;
}
