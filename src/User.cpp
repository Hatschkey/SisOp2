#include "User.h"

std::map<std::string, User*> User::active_users;
RW_Monitor User::active_users_monitor;

User* User::getUser(std::string username)
{
    User* user; // Reference to the user

    try
    {
        // Request read rights
        active_users_monitor.requestRead();

        // Try to find username in map
        user = active_users.at(username);
    }
    catch(const std::out_of_range& e)
    {
        // If reached end of map, user is not there
        user = NULL;
    }

    // Release read rights
    active_users_monitor.releaseRead();

    return user;
};

void User::addUser(User* user)
{
    // Request write rights
    active_users_monitor.requestWrite();

    // Insert user in map
    active_users.insert(std::make_pair(user->username,user));

    // Release write rights
    active_users_monitor.releaseWrite();
}

int User::removeUser(std::string username)
{
    // Request write rights
    active_users_monitor.requestWrite();

    // Remove user from map
    return active_users.erase(username);

    // Release write rights
    active_users_monitor.releaseWrite();
}

void User::listUsers()
{
    // Request read rights
    active_users_monitor.requestRead();

    // Iterate map listing users
    for (std::map<std::string,User*>::iterator i = active_users.begin(); i != active_users.end(); ++i)
    {
        std::cout << std::endl;
        std::cout << "Username: " << i->second->username << std::endl;
        std::cout << "Active sessions: " << i->second->getSessionCount() << std::endl;
        std::cout << "Last seen: " << std::ctime((time_t*)&(i->second->last_seen)) << std::endl;
    }

    // Release read rights
    active_users_monitor.releaseRead();
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

    // Request read rights
    joined_groups_monitor.requestRead();

    // Iterate map summing all user sessions
    for (std::map<std::string,int>::iterator i = this->joined_groups.begin(); i != this->joined_groups.end(); ++i)
    {
        total_sessions += i->second;
    }

    // Release read rights
    joined_groups_monitor.releaseRead();

    return total_sessions;
}

int User::joinGroup(Group* group, int socket_id)
{
    // Check for user session count
    if (this->getSessionCount() < MAX_SESSIONS)
    {
        // Add user to group
        group->addUser(this);

        // Request write rights
        joined_groups_monitor.requestWrite();

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

        // Release write rights
        joined_groups_monitor.releaseWrite();

        // Request write rights
        group_sockets_monitor.requestWrite();

        // Add socket descriptor to corresponding thread id list
        group_sockets.insert(std::make_pair(socket_id,group->groupname));

        // Release write rights
        group_sockets_monitor.releaseWrite();

        // Return sucessful join
        return 1;
    }

    // If user has 2 active sessions, refuse to join group
    return 0;
}

int User::leaveGroup(Group* group, int socket_id)
{
    // Request write rights
    joined_groups_monitor.requestWrite();

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
            // Request write rights
            active_users_monitor.requestWrite();

            // Remove user from static user list
            User::active_users.erase(this->username);

            // Release write rights
            active_users_monitor.releaseWrite();
            joined_groups_monitor.releaseWrite();

            // Request write rights
            group_sockets_monitor.requestWrite();

            // Remove session thread from vector
            group_sockets.erase(socket_id);

            // Release write rights
            group_sockets_monitor.releaseWrite();

            // And delete itself
            delete(this);

        }
        else
        {
            // Release write rights
            joined_groups_monitor.releaseWrite();
        }

    }
    else
    {
        // Release write rights
        joined_groups_monitor.releaseWrite();
    }

    return 0;
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

int User::signalNewMessage(std::string message, std::string username, std::string groupname, int packet_type)
{
    int message_record_size = 0; // Size of the composed message (in bytes)

    // Compose a new message
    message_record* msg = BaseSocket::composeMessage(username, message, 0 /* HERE!*/ );

    // Calculate message size
    message_record_size = sizeof(*msg) + msg->length;

    // Request read rights
    group_sockets_monitor.requestRead();

    // Iterate through connected client sockets
    for (std::map<int, std::string>::iterator i = group_sockets.begin(); i != group_sockets.end(); ++i)
    {
        // If client is part of this group, send message
        if (groupname.compare(i->second) == 0)
            BaseSocket::sendPacket(i->first, packet_type, (char*)msg, message_record_size);
    }

    // Release read rights
    group_sockets_monitor.releaseRead();

    // Free created structure
    free(msg);

    return 1;
}
