#include "ReplicaManager.h"

/* Replica entrypoint */
int main(int argc, char **argv)
{
    // Parse command line input
    if (argc < 7)
    {
        std::cerr << "Usage: " << argv[0] << " <N> <replica-port> <replica-ID> <leader-ip> <leader-port> <leader-id> " << std::endl;
        return 1;
    }

    try
    {
        // Create an instance of Replica
        ReplicaManager replica(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), argv[4], atoi(argv[5]), atoi(argv[6]));

        // Start listening for connections
        replica.listenConnections(NULL);
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << '\n';
    }

    // End
    std::cout << "Replica " << argv[2] << " stopped safely." << std::endl;
    return 0;
}