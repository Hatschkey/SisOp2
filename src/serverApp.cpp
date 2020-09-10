#include "Server.h"

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