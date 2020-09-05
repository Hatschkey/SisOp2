#include <pthread.h>

#include "Client.h"

/* 
 * Client entrypoint 
 */
int main(int argc, char** argv)
{

    // Parse command line input
    if (argc < 5){
        std::cerr << "Usage: " << argv[0] << " <username> <groupname> <server_ip_address> <port> " << std::endl;
        return 1;
    }

    try
    {
        // Create an instance of Client
        Client client(argv[1],argv[2],argv[3],argv[4]);

        // Setup a connection to the server
        client.setupConnection();

        // Send first packet (login packet)
        client.sendLoginPacket(); 

        // TODO Start thread to get messages from the server

        // Get user messages to be sent until Ctrl D is pressed
        std::string user_message;
        while(std::getline(std::cin, user_message))
        {
            try
            {
                client.sendMessagePacket(user_message);
            }
            catch(const std::runtime_error& e)
            {
                std::cerr << e.what() << std::endl;
            }
            
        }

        // TODO Signal server-listening thread to end

        // TODO pthread_join that thread
        
    }
    catch(const std::invalid_argument& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    // Exit with return code 0
	return 0;
}
