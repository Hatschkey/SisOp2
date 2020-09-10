#include "User.h"

std::map<std::string, User*> User::active_users;

User* User::getUser(std::string username)
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

void User::addUser(User* user)
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
    for (std::map<std::string,User*>::iterator i = active_users.begin(); i != active_users.end(); ++i)
    {
        std::cout << std::endl;
        std::cout << "Username: " << i->second->username << std::endl;
        std::cout << "Active sessions: " << i->second->getSessionCount() << std::endl;
        std::cout << "Last seen: " << i->second->last_seen << std::endl;
    }
    
}

User::User(std::string username)
{
    // Update username, last seen and active sessions
    this->username = username;
    this->last_seen = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    // Add itself to the active user list
    User::addUser(this);
}

int User::getSessionCount()
{
    int total_sessions = 0; // Total sessions the user has at the moment

    // Iterate map summing all user sessions
    for (std::map<std::string,int>::iterator i = this->joined_groups.begin(); i != this->joined_groups.end(); ++i)
    {
        total_sessions += i->second;
    }

    return total_sessions;
}

int User::joinGroup(Group* group)
{
    // Check for user session count 
    if (this->getSessionCount() < MAX_SESSIONS)
    {
        // Add user to group
        group->addUser(this);

        // Check if this user is already connected to this group
        if (this->joined_groups.find(group->groupname) == this->joined_groups.end()) 
        {
            // If not, add it with 1 session
            this->joined_groups.insert(std::make_pair(group->groupname, 1));
        }
        else
        {
            // If it is, increase session count
            this->joined_groups.at(group->groupname)++;
        }

        // Return sucessful join
        return 1;
    }

    // If user has 2 active sessions, refuse to join group
    return 0;
}

void User::leaveGroup(Group* group)
{
    // Remove the group from this user's group list
    this->joined_groups.at(group->groupname)--;

    // Check if any sessions are left in that group
    if (this->joined_groups.at(group->groupname) == 0)
    {
        // If not, remove the group from the joined groups map
        this->joined_groups.erase(group->groupname);

        // Leave the group
        group->removeUser(this->username);

        // Check if this was the last session for the user
        if (this->joined_groups.empty())
        {
            // Remove user from static user list
            User::active_users.erase(this->username);

            // And delete itself
            free(this);
        }
    }
}

int User::say(std::string message, std::string groupname)
{
    // Fetch the group
    Group* group = Group::getGroup(groupname);

    // "Posts" a message to group, along with sender username
    if (group != NULL)
        return group->post(message, this->username);

    return 0;
}

