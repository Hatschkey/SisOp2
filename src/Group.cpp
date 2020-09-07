#include "Group.h"

std::map<std::string, group_t*> Group::active_groups;

group_t* Group::getGroup(std::string groupname)
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

void Group::addGroup(group_t* group)
{
    // Insert in group map
    active_groups.insert(std::make_pair(group->groupname, group));
}

int Group::removeGroup(std::string groupname)
{
    // Get group from map
    //group_t* group = active_groups.at(groupname);

    // TODO Close open group history file

    // Remove group from map
    return active_groups.erase(groupname);
}

void Group::listGroups()
{
    // Iterate map listing groups and their users
    for (std::map<std::string,group_t*>::iterator i = active_groups.begin(); i != active_groups.end(); ++i)
    {
        std::cout << std::endl;
        std::cout << "Groupname: " << i->second->groupname << std::endl;
        std::cout << "User count: " << i->second->user_count << std::endl;
        // TODO Keep track of users inside groups
    }
}