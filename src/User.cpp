#include "User.h"

std::map<std::string, user_t*> User::active_users;

user_t* User::getUser(std::string username)
{

    try
    {
        // Try to find username in map
        return active_users.at(username);
    }
    catch(const std::out_of_range& e)
    {
        // If reached end of map, user is not there
        return NULL;
    }
        
};

void User::addUser(user_t* user)
{
    // Insert user in map
    active_users.insert(std::make_pair(user->username,user));
}

int User::removeUser(std::string username)
{
    // Remove user from map
    return active_users.erase(username);
}

void User::listUsers()
{
    // Iterate map listing users
    for (std::map<std::string,user_t*>::iterator i = active_users.begin(); i != active_users.end(); ++i)
    {
        std::cout << std::endl;
        std::cout << "Username: " << i->second->username << std::endl;
        std::cout << "Active sessions: " << i->second->active_sessions << std::endl;
        std::cout << "Last seen: " << i->second->last_seen << std::endl;
    }
    
}