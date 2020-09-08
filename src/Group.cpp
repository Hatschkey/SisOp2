#include "Group.h"

std::map<std::string, Group*> Group::active_groups;

Group::Group(std::string groupname)
{
    // Update groupname
    this->groupname = groupname;

    // TODO Update eventual other variables

    // Add itself to group list
    Group::addGroup(this);
}

Group::~Group()
{
    // TODO Close any open group files
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