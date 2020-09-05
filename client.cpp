#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "structures.h"

using namespace std;

int validate_credentials(std::string username, std::string groupname)
{

    /* Check if username is valid */
    if ( !std::regex_match(username,std::regex(NAME_REGEX)) )
    {
        std::cerr << "Usernames must be between 4-20 digits containing only [a-zA-Z0-9.] Usernames must start with a letter" << std::endl;
        return ERROR_INVALID_USERNAME;
    }
    
    /* Check if groupname is valid*/
    if ( !std::regex_match(groupname,std::regex(NAME_REGEX)) )
    {
        std::cerr << "Groupnames must be between 4-20 digits containing only [a-zA-Z0-9.] Group must start with a letter" << std::endl;
        return ERROR_INVALID_GROUPNAME;
    }   
    
    return 0;
}

/* Client entrypoint */
/* $ ./client <username> <groupname> <server_ip_address> <port> */
int main(int argc, char** argv)
{

    string username;             // Client username
    string groupname;            // Group name
    string server_ip_address;    // Server IP
    int    server_port = -1;     // Server Port
    int    status = 0;           // Exit status

    /* Parse command line input */
    if (argc < 2){
        cerr << "Usage: " << argv[0] << " <username> <groupname> <server_ip_address> <port> " << endl;
        return 1;
    }
    
    /* Get session info for the user */
    username          = argv[1];
    groupname         = argv[2];
    server_ip_address = argv[3];
    server_port       = atoi(argv[4]);
    
    /* Validade username, groupname and server ip/port */
    if ( (status = validate_credentials(username, groupname)) < 0) return status;
    
    /* Create socket for server communication */
    int connection_socket;
    struct sockaddr_in server_address;
    
    if ( (connection_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "Error during socket creation: " << connection_socket << std::endl;
        return ERROR_SOCKET_CREATION;
    }
    
    server_address.sin_family = AF_INET;            // Connect via IPv4
    server_address.sin_port   = htons(server_port); // On port specified by server_port
    server_address.sin_addr.s_addr = inet_addr(server_ip_address.c_str());
    
    /* Try to connect to server */
    if (connect(connection_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        std::cerr << "Connection to address " << server_ip_address << " failed. IP might be invalid or server might be down." << std::endl;
        return ERROR_CONNECTION_FAILED;
    }
    
    /* At this point, connection has been established with the server, and the client may start sending packets */
    
    /* Send packet to server with information about the group and username */
    
    /* Prepare content payload: "username,groupname"*/
    login_payload* payload = (login_payload*)malloc(sizeof(login_payload)); 
    strcpy(payload->username,  &username[0]);
    strcat(payload->username, "\0");
    strcpy(payload->groupname, &groupname[0]);
    strcat(payload->groupname, "\0");

    int payload_size = sizeof(*payload);

    /* Allocate packet memory */
    packet* user_information_packet = (packet*)malloc(sizeof(*user_information_packet) + sizeof(char) * payload_size);
    user_information_packet->type      = PAK_CMD; // Signal that a command packet is being sent
    user_information_packet->sqn       = 0;       // Sequence number is 0 for the first packet
    user_information_packet->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp 
    user_information_packet->length    = payload_size;
    memcpy((void*)user_information_packet->_payload, (void*)payload, payload_size);

    // Calculate packet size
    int packet_size = sizeof(*user_information_packet) +  payload_size;

    // Send data to server
    std::cout << "Sent " << send(connection_socket, user_information_packet, packet_size, 0) << " bytes to server" << std::endl;
    std::cout << "Logged in as " << username << " in group " << groupname <<std::endl;
    std::cout << "     Say something: ";

    /* TODO Get packet(s) from server in order to display group chat and old messages */

    /* TODO Just testing sending packets through sockets */
    std::string user_message;
    while (std::getline(std::cin, user_message))
    {

        // Allocate payload data
        char* payload = &user_message[0];
        int payload_size = strlen(payload) + 1;

        // Allocate packet data
        packet* data = (packet*)malloc(sizeof(*data) + sizeof(char) * payload_size);
        data->type      = PAK_DAT;
        data->sqn       = 1;
        data->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        strcpy((char*)data->_payload, payload);
        strcat((char*)data->_payload,"\0");
        data->length = payload_size;
        
        // Calculate packet size
        int packet_size = sizeof(*data) + (sizeof(char) * payload_size);

        // Send data to server
        std::cout << "Sent " << send(connection_socket, data, packet_size, 0) << " bytes to server" << std::endl;
        std::cout << "'data' structure has size " << sizeof(*data) << " bytes" << std::endl;
        std::cout << "Payload is '" << data->_payload << "' and has size " << payload_size << std::endl;
    
        free(data);

        std::cout << "     Say something: ";
    }
    /*TODO*/
    
    /* Close connection to server */
    close(connection_socket);
    
    std::cout << std::endl;
	return status;
}
