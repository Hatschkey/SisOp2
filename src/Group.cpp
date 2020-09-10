#include "Group.h"

std::map<std::string, Group*> Group::active_groups;

Group::Group(std::string groupname)
{
    // Update groupname
    this->groupname = groupname;

    // Try to open group history file (groupname.hist)
    this->history_file = fopen(groupname.append(".hist").c_str(),"ab+");

    // Add itself to group list
    Group::addGroup(this);
}

Group::~Group()
{
    // Close group history file
    fclose(this->history_file);
}

Group* Group::getGroup(std::string groupname)
{
    try
    {
        // Try to find groupname in map
        return active_groups.at(groupname);
    }
    catch(const std::out_of_range& e)
    {
        // If reached end of map, group is not there
        return NULL;
    }
    
}

void Group::addGroup(Group* group)
{
    // Insert in group map
    active_groups.insert(std::make_pair(group->groupname, group));
    
}

int Group::removeGroup(std::string groupname)
{
    // Remove group from map
    return active_groups.erase(groupname);
}

void Group::listGroups()
{
    // Iterate map listing groups and their users
    for (std::map<std::string,Group*>::iterator i = active_groups.begin(); i != active_groups.end(); ++i)
    {
        std::cout << std::endl;
        std::cout << "Groupname: " << i->second->groupname << std::endl;
        std::cout << "User count: " << i->second->getUserCount() << std::endl;
        // List all users in the group
        i->second->listUsers();
    }
}

void Group::addUser(User* user)
{
    // Insert user in map
    users.insert(std::make_pair(user->username, user));
}

int Group::removeUser(std::string username)
{
    int removed_users = 0; // Amount of users that was removed

    // Erase user from vector
    removed_users = users.erase(username);

    // Check how many users are left in the group
    if (this->users.size() == 0)
    {
        // If no users are left, remove itself from the static list
        Group::active_groups.erase(this->groupname);

        // And delete itself
        free(this);
    }

    return removed_users;
}

void Group::listUsers()
{
    // Iterate vector listing all users
    for (std::map<std::string, User*>::iterator i = users.begin(); i != users.end(); ++i)
        std::cout << "User: " << i->second->username << std::endl;

}

int Group::getUserCount()
{
    return users.size();
}

int Group::post(std::string message, std::string username)
{
    // Create a record for the user message
    message_record* msg = (message_record*)malloc(sizeof(message_record));
    sprintf(msg->username,"%s", username.c_str());  // Copy username
    msg->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp
    msg->length = strlen(message.c_str()) + 1; // Update message length
    sprintf((char*)msg->_message,"%s",message.c_str()); // Copy message

    // Calculate struct size
    int record_size = sizeof(*msg) + sizeof(char) * strlen(message.c_str());

    // Write message in group history file
    fwrite((void*)msg, record_size, 1, history_file);

    // Send message to every connected user (Including message sender)
    for (std::map<std::string, User*>::iterator i = users.begin(); i != users.end(); ++i)
    {
        // TODO Placeholder debug message
        std::cout << "TODO Send message to " << i->second->username << std::endl;
    }

    // Free memory used for message record
    free(msg);

    // TODO Change here to return the amount of messages that were issued
    return users.size();
}