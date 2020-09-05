#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "structures.h"


/* Message Manager Thread */
void *MessageManager(void* arg);
/* Group Manager Thread */
void *GroupManager(void* arg);
/* Communication Manager Thread */
void *CommManager(void* arg);
/* Server command manager Thread */
void *ServerManager(void* arg);

/* Thread error hander, stops application */
void handle_error(int status);

/* Returns a reference to user if it is connected, NULL otherwise */
user_t *user_connected(std::string username);
/* Returns a reference to group if it is currently active, or creates group if it isn't */
group_t *group_active(std::string groupname);

/* Shared data between threads*/
/* TODO Mutex lock this variable*/
managed_data_t shared_data;


/* Server entrypoint */
/* $ ./server <N> */
int main(int argc, char** argv)
{

    /* Server socket - related variables */
    int server_socket, client_socket;                   // Server and client sockets
    int sockaddr_size;                                  // Size of sockaddr_in struct
    int* new_socket;                                    // Socket reference to client
    struct sockaddr_in server_address, client_address;  // Server and client addresses

    /* Manager module threads */
    pthread_t message_manager, group_manager, server_command_manager;
    std::vector<pthread_t> communication_threads; // List of all communication threads

    /* Parse command line input */
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <N> " << std::endl;
        return 1;
    }
    
    /* Get N */
    int N = atoi(argv[1]);
 
    pthread_create(&message_manager, NULL, MessageManager, NULL); // Message Manager (Group text messages, message metadata)
    pthread_create(&group_manager  , NULL, GroupManager  , NULL); // Group Manager (Groups, users)
    pthread_create(&server_command_manager, NULL, ServerManager, NULL); // Server command manager (Stop server, etc)

    /* Create server socket */
    if ( (server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        std::cerr << "Error during socket creation: " << server_socket << std::endl;
        return ERROR_SOCKET_CREATION;
    }
    
    /* Prepare server address */
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);
    
    /* Set socket options*/
    int yes = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        std::cerr << "Error setting socket options" << std::endl;
        return ERROR_SOCKET_OPTION;
    }
    
    /* Bind socket */
    if ( bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0 )
    {
        std::cerr << "Error during socket bind" << std::endl;
        return ERROR_SOCKET_BIND;
    }
    
    /* Listen at socket */
    listen(server_socket, 3);

    /* Wait for incoming connections */
    std::cout << "[Server:Main] Ready to receive connections" << std::endl;
    sockaddr_size = sizeof(struct sockaddr_in);
    while(!shared_data.stop_issued && (client_socket = accept(server_socket, (struct sockaddr*)&client_address, (socklen_t*)&sockaddr_size)) )
    {
        std::cout << "[Server:Main] New connection accepted" << std::endl;
        
        /* Start new thread for new connection */
        pthread_t comm_thread;
        
        /* Get reference to client thread */
        new_socket = (int*)malloc(1);
        *new_socket = client_socket;
        
        if (pthread_create( &comm_thread, NULL, CommManager, (void*)new_socket) < 0)
        {
            std::cerr << "Could not create thread for socket " << client_socket << std::endl;
            return ERROR_THREAD_CREATION;
        }
          
        /* Add thread to list */
        communication_threads.push_back(comm_thread);
    }
    
    close(server_socket);
    
    std::cout << "[Server:Main] Waiting for threads to end..." << std::endl;
    
    /* Wait for threads to finish */
    int *thread_status; // Returned value by threads
    pthread_join(message_manager, (void**)&thread_status);
    std::cout << "[Server:Main] Message manager finished with status " << *thread_status << std::endl;
    pthread_join(group_manager  , (void**)&thread_status);
    std::cout << "[Server:Main] Group manager finished with status " << *thread_status << std::endl;
    
    /* TODO Do something to stop client comunication threads*/

    /* Wait for each client communication to end */
    for (std::vector<pthread_t>::iterator i = communication_threads.begin(); i != communication_threads.end(); ++i)
    {
        std::cout << "[Server:Main] Waiting for client communication to end..." << std::endl;
        pthread_t* ref = &(*i);
        pthread_join(*ref, NULL);
    }


    /* End */
    std::cout << "[Server:Main] Server finishing..." << std::endl;
	return 0;
}

/*
    MESSAGE MANAGER
*/
void *MessageManager(void* arg)
{

    /* TODO Setup*/
    
    while(!shared_data.stop_issued)
    {
        //std::cout << "Message manager running" << std::endl;  
    }
    
    /* TODO Cleanup*/
    
    /* Finish thread */
    handle_error(0);
}

/*
    GROUP MANAGER
*/
void *GroupManager(void* arg)
{

    /* TODO Setup*/
        

    while(!shared_data.stop_issued)
    {
        //std::cout << "Group manager running" << std::endl;  
    }
    
    /* TODO Cleanup*/

    /* Finish thread*/
    handle_error(0);
}

/*
    COMMUNICATION MANAGER
*/
void *CommManager(void* arg)
{
    /* Get client socket */
    int socket = *(int*)arg;
    int read_bytes = 0;                  // Number of bytes read from the message
    char client_message[PACKET_MAX];     // Buffer for client message, maximum of PACKET_MAX bytes
    login_payload* login_payload_buffer; // Buffer for client login information
    std::string message;                 // User chat message

    /* User connected through this thread and group they are connected to */
    user_t* user = NULL;
    group_t* group = NULL;

    /* TODO Perform some validations */

    /* Wait for messages from client*/
    while( (read_bytes = recv(socket, client_message, PACKET_MAX, 0)) > 0 && !shared_data.stop_issued )
    {
        // Decode received data into a packet structure
        packet* received_packet = (packet *)client_message;
        
        // Print packet info to screen
        std::cout << "[Server:CommManager] Received packet of type " << received_packet->type << " with " << read_bytes << " bytes" << std::endl;
        //std::cout << "Sequence:  " << received_packet->sqn << std::endl;
        //std::cout << "Length:    " << received_packet->length << std::endl;

        /* Check packet type */
        switch (received_packet->type)
        {
            /* Command packet: register user and group*/
            case PAK_CMD:
                
                std::cout << "[Server:CommManager] Received login packet from client: " << std::endl;

                /* Get user login information*/         
                login_payload_buffer = (login_payload*)received_packet->_payload;
                
                std::cout << "Username: " << login_payload_buffer->username;
                std::cout << ", groupname: " << login_payload_buffer-> groupname << std::endl;

                /* Check if user exists in the active list*/
                if (( user = user_connected(login_payload_buffer->username) )!= NULL)
                {
                    /* User is already connected, increase session count */
                    /* TODO Mutex */
                    /* If user has less than 2 active sessions, allow them to connect */
                    if (user->active_sessions < 2) user->active_sessions++;
                    else 
                    {
                        /* TODO Send packet to user informing max 2 sessions */

                        /* Close client socket */
                        close(socket);

                        /* Free client socket pointer */
                        free(arg);

                        std::cout << "[Server:CommManager] Client forcefully disconnected - Max "<< MAX_SESSIONS <<" sessions" << std::endl;
                        
                        /* Exit thread */
                        handle_error(0);
                    }
                }
                else
                {
                    /* Create new user and initialize data */
                    user = (user_t*)malloc(sizeof(user_t));
                    user->active_sessions = 1;
                    user->last_seen = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                    strcpy(user->username, login_payload_buffer->username);

                    /* TODO MUTEX Add to active user list */
                    shared_data.active_users.push_back(*user);
                }

                /* Get the group information */
                group = group_active(login_payload_buffer->groupname);
                /* TODO Update user-group map */

                break;
            /* Data packet: User messages to connected group */
            case PAK_DAT:
                std::cout << "[" << user->username << " @ " << group->groupname << "] says: " << received_packet->_payload << std::endl;

                break;
            default:
                break;
        }

        /* Clear buffer for next packets*/
        for (int i=0; i < PACKET_MAX; i++) client_message[i] = '\0';

    }
    /* If client disconnected */
    if (read_bytes == 0) 
    {
        std::cout << "[" << user->username << " @ " << group->groupname << "] disconnected" << std::endl;
        user->active_sessions--;
        
        /* If no active sessions for the user are left, free user space */
        if (user->active_sessions == 0)
        {
            /* TODO Mutex */
            free(user);
        }
    }

    /* Check if this was last user in group */
    if (group != NULL && group->user_count == 0) fclose(group->history);

    /* Free client socket pointer */
    free(arg);

    /* Finish with status code 0 */
    handle_error(0);
}

void *ServerManager(void* arg)
{
    /* Get commands from server administrator */
    std::string user_command;
    while(std::getline(std::cin, user_command))
    {
        /* TODO Process commands*/
    }

    /* TODO mutex */
    shared_data.stop_issued = 1;
    pthread_exit(NULL);
}

void handle_error(int status)
{
    /* Get return value for exiting thread */
    int* retval = (int*)malloc(sizeof(int));
    *retval = status;

    /* Exit thread */
    pthread_exit(retval);
}

/* TODO MUTEX */
/* TODO Change vector to set, string to char, etc */
user_t *user_connected(std::string username)
{
    for (std::vector<user_t>::iterator i = shared_data.active_users.begin(); i != shared_data.active_users.end(); ++i)
    {
        if (!username.compare(i->username))
        {
            return &(*i);   
        }
    }

    return NULL;
}

/* TODO MUTEX */
/* TODO Same as user_connected*/
group_t *group_active(std::string groupname)
{
    FILE* history;
    
    /* Check if group history exists */    
    if ( (history = fopen((groupname + ".hist").c_str(), "r+")) ==  NULL)
    {
        /* If history does not exist, create group */
        group_t* new_group = (group_t*)malloc(sizeof(group_t));
        strcpy(new_group->groupname, groupname.c_str());
        new_group->history = fopen((groupname + ".hist").c_str(), "w+");
        new_group->user_count = 1;

        /* TODO Mutex */
        shared_data.active_groups.push_back(*new_group);

        return new_group;
    }
    else
    {
        for (std::vector<group_t>::iterator i = shared_data.active_groups.begin(); i != shared_data.active_groups.end(); ++i)
        {
            if (!groupname.compare(i->groupname))
            {
                /* Group exists in active list, return ref */
                return &(*i);
            }
        }
    }
}