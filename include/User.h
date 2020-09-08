#include <string>
#include "data_types.h"

class User
{

    public:

    static std::map<std::string, user_t*> active_users;   // Current active users 

    /**
     * Searches for the given username in the currently active users list. 
     * TODO This method could be read-write protected, in order to allow only 1 writer at a time or multiple readers at a time
     * @param username Username of the user to search for
     * @return Reference to the user structure in the user list
     */
    static user_t* getUser(std::string username);

    /**
     * Add user to currently active user list
     * TODO This method also should be read-write protected, only one user may be added at a time
     * @param user Pointer to user that will be added to list
     */
    static void addUser(user_t* user);

    /**
     * Remove specified user
     * @param username Name of the user that should be removed from this list
     * @return Amount of removed users, should always be either 1 or 0
     */
    static int removeUser(std::string username);

    /**
     * Debug function, lists all active users to stdout
     */
    static void listUsers();

};