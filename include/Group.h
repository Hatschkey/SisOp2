#include <map>

#include "constants.h"
#include "data_types.h"

class Group
{

    public: 

    static std::map<std::string, group_t*> active_groups;  // Current active groups 

    /**
     * Searches for the given groupname in the currently active group list
     * @param groupname Name of the group to search for
     * @return Reference to the group structure in the group list
     */
    static group_t* getGroup(std::string groupname);
    
    /**
     * Add group to currently active group list
     * @param group Pointer to the group that will be added to the list
     */
    static void addGroup(group_t* group);
    
    /**
     * Remove specified group
     * @param groupname Name of the group that should be removed from this list
     * @return Number of removed groups, should always be either 1 or 0
     */
    static int removeGroup(std::string groupname);
    
    /**
     * Debug function, lists all active groups and their current users
     */
    static void listGroups(); 
};