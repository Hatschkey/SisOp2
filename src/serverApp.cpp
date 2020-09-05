#include <pthread.h>

#include "Server.h"

/* Returns a reference to user if it is connected, NULL otherwise */
user_t *user_connected(std::string username);
/* Returns a reference to group if it is currently active, or creates group if it isn't */
group_t *group_active(std::string groupname);

/* Server entrypoint */
int main(int argc, char** argv)
{
    // Parse command line input
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <N> " << std::endl;
        return 1;
    }

    try
    {
        // Create an instance of Server
        Server server(atoi(argv[1]));

        // Start listening to connections
        server.listenConnections();
        
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << e.what() << std::endl;
    }

    // End
    std::cout << "Server stopped safely" << std::endl;
	return 0;
}