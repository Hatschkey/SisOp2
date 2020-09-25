#ifndef GROUP_H
#define GROUP_H

#include <map>
#include <string.h>
#include <unistd.h>
#include <list>

#include "constants.h"
#include "data_types.h"
#include "User.h"
#include "RW_Monitor.h"
#include "CommunicationUtils.h"


// Forward declare User
class User;

class Group : protected CommunicationUtils
{
    public:
    static std::map<std::string, Group*> active_groups;  // Current active groups
    static RW_Monitor active_groups_monitor;             // Monitor for the group list variable

    std::string groupname;                  // Name for this group instance
    std::map<std::string, User*> users;     // Map of references to users connected to this group
    RW_Monitor users_monitor;               // Monitor for this instance's user list
    FILE* history_file;                     // File descriptor for this group's history file
    RW_Monitor history_file_monitor;        // Monitor for acessing the group history file

    // These static methods are related to the list of all groups (static active_groups)
    /**
     * Searches for the given groupname in the currently active group list
     * @param groupname Name of the group to search for
     * @return Reference to the group structure in the group list
     */
    static Group* getGroup(std::string groupname);

    /**
     * Add group to currently active group list
     * @param group Pointer to the group that will be added to the list
     */
    static void addGroup(Group* group);

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

    // These non-static methods are related to an instance of group

    /**
     * Class constructor
     * @param groupname Name of the group that will be created
     */
    Group(std::string groupname);

    /**
     * Class destructor, closes any open history files left by this group
     */
    ~Group();

    /**
     * Add given user to group
     * @param user User to be added to this group
     */
    void addUser(User* user);

    /**
     * Remove the user corresponding to the given username
     * @param username Name of the user that should be removed from this group
     * @return 1 if this was the last user in group, 0 otherwise
     */
    int removeUser(std::string username);

    /**
     * Debug function, list all users that are part of the group to stdout
     */
    void listUsers();

    /**
     * Gets current user count for this group
     * @returns Current user count
     */
    int getUserCount();

    /**
     * "Posts" a chat message in this group
     * Saves the message and notifies all group members, including sender
     * @param message  Message that is being posted in the group
     * @param username Who sent this message
     * @return Number of users this message was sent to, should always be at least 1 (the sender) on success
     */
    int post(std::string message, std::string username);

    /**
     * Creates and saves the given message to this groups history file.
     * TODO This method should be mutex protected (No other threads should read or write to the file concurrently)
     * @param message  Message that will be saved
     * @param username User who sent this message
     */
    void saveMessage(std::string message, std::string username);

    /**
     * Recovers the last N messages from the group's history file, sending them to the user
     * @param message_record_list Buffer for reading recorded messages history
     * @param n    Number of messages that will be recovered
     * @param user Pointer to the user instance that will receive these messages
     * @return Number of recorded messages retrieved from the grupo.hist file
     */
    int recoverHistory(char* message_record_list, int n, User* user);

    /**
     * Brodcast a chat message to connected clients as the server
     * Saves the message and notifies all group members, including sender
     * @param message  Message that is being broadcasted to the clients   
     * @param username Username name for the client logging in or out
     * @return Number indicanting success or failure(0 or 1)
     */
    int broadcastMessage(std::string message, std::string username);

};

#endif