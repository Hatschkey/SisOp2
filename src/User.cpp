#include "User.h"

std::map<std::string, User *> User::active_users;
RW_Monitor User::active_users_monitor;

User *User::getUser(std::string username)
{
    User *user; // Reference to the user
    int exists = 0;

    try
    {
        // Try to find username in map
        user = active_users.at(username);

        exists = 1;
    }
    catch (const std::out_of_range &e)
    {
        // If reached end of map, user is not there
        exists = 0;
    }

    return exists ? user : new User(username);
};

void User::addUser(User *user)
{
    // Insert user in map
    active_users.insert(std::make_pair(user->username, user));
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

    // Delimiter
    std::cout << "======================\n"
              << std::endl;

    // Iterate map listing users
    for (std::map<std::string, User *>::iterator i = active_users.begin(); i != active_users.end(); ++i)
    {
        std::cout << "Username: " << i->second->username << std::endl;
        std::cout << "Active sessions: " << i->second->getSessionCount() << std::endl;
        std::cout << "Last seen: " << std::ctime((time_t *)&(i->second->last_seen)) << std::endl;
    }

    // Delimiter
    std::cout << "======================" << std::endl;

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
    session_monitor.requestRead();

    // Iterate map summing all user sessions
    for (auto i = this->sessions.begin(); i != this->sessions.end(); ++i)
    {
        total_sessions++;
    }

    // Release read rights
    session_monitor.releaseRead();

    return total_sessions;
}

int User::getSessionCount(std::string groupname)
{
    int count = 0;

    this->session_monitor.requestRead();

    // Iterate sessions
    for (auto i = this->sessions.begin(); i != this->sessions.end(); ++i)
    {
        if (!i->second->getGroup()->groupname.compare(groupname))
        {
            count++;
        }
    }

    this->session_monitor.releaseRead();

    return count;
}

int User::joinGroup(Group *group, Session *session)
{
    // Login message
    std::string message;

    // Check for user session count
    if (this->getSessionCount() < MAX_SESSIONS)
    {

        // Check if this user is already connected to this group
        if (this->getSessionCount(group->groupname) == 0)
        {
            // Add user to group
            group->addUser(this);

            // Send a login message
            message = "User [" + this->username + "] has joined.";
            group->post(message, this->username, SERVER_MESSAGE);
        }

        // Request write rights
        this->session_monitor.requestWrite();

        // Add to list
        this->sessions.insert(std::make_pair(session->getId(), session));

        // Release write rights
        this->session_monitor.releaseWrite();

        // Return sucessful join
        return 1;
    }

    // If user has 2 active sessions, refuse connection
    return 0;
}

int User::leaveGroup(Session *session)
{
    // Request write rights
    this->session_monitor.requestWrite();

    // Remove the session from the user list
    this->sessions.erase(session->getId());

    // Release write rights
    this->session_monitor.releaseWrite();

    // Check if this was the last user session from the leaving group
    if (this->getSessionCount(session->getGroup()->groupname) == 0)
    {
        // Send a disconnect message
        std::string message = "User [" + username + "] has disconnected.";
        session->getGroup()->post(message, username, SERVER_MESSAGE);

        // Leave the group
        session->getGroup()->removeUser(this->username);

        // Check if this is the last user session at all
        if (this->getSessionCount() == 0)
        {
            // If so, remove itself from active users list
            User::active_users_monitor.requestWrite();
            User::active_users.erase(this->username);
            User::active_users_monitor.releaseWrite();

            // And delete itself
            delete this;
        }
    }

    return 0;
}

int User::signalNewMessage(std::string message, std::string username, std::string groupname, int packet_type, int message_type)
{
    int message_record_size = 0; // Size of the composed message (in bytes)

    // Compose a new message
    message_record *msg = CommunicationUtils::composeMessage(username, message, message_type);

    // Request read rights
    session_monitor.requestRead();

    // Iterate through sessions
    for (auto i = sessions.begin(); i != sessions.end(); ++i)
    {
        // If client is part of this group, send message
        if (!i->second->getGroup()->groupname.compare(groupname))
            i->second->messageClient(msg, packet_type);
    }

    // Release read rights
    session_monitor.releaseRead();

    // Free created structure
    free(msg);

    return 1;
}

void User::setLastSeen()
{
    // Update variable
    this->last_seen = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}