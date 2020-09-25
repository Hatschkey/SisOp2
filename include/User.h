#ifndef USER_H
#define USER_H

#include <string>
#include <chrono>
#include <vector>
#include <pthread.h>

#include "constants.h"
#include "data_types.h"
#include "Group.h"
#include "RW_Monitor.h"
#include "BaseSocket.h"

// Forward declare Group
class Group;

class User : protected BaseSocket
{
    public:
    static std::map<std::string, User*> active_users;   // Current active users
    static RW_Monitor active_users_monitor; // Monitor for active users

    std::string username; // User display name
    int last_seen; // Last time a message was received from this user
    std::map<std::string, int> joined_groups; // Groups this user instance has joined and how many sessions are active in each group
    RW_Monitor joined_groups_monitor;   // Monitor for joined groups
    
    std::map<int, std::string> group_sockets; // Map for group and sockets related to that group (Max MAX_SESSIONS)
    RW_Monitor group_sockets_monitor;  // Monitor for group to socket id map

    /**
     * Searches for the given username in the currently active users list.
     * TODO This method could be read-write protected, in order to allow only 1 writer at a time or multiple readers at a time
     * @param username Username of the user to search for
     * @return Reference to the user structure in the user list
     */
    static User* getUser(std::string username);

    /**
     * Add user to currently active user list
     * TODO This method also should be read-write protected, only one user may be added at a time
     * @param user Pointer to user that will be added to list
     */
    static void addUser(User* user);

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

    /**
     * Class constructor
     * @param username Display name for the user
     */
    User(std::string username);

    /**
     * Returns the current session count for this user instance
     * @return Amount of active user sessions
     */
    int getSessionCount();

    /**
     * Tries to join the given group, if the session count allows for it
     * @param group Instance of a Group class
     * @param socket_id Identifier for the socket corresponding to the session thread
     * @return 1 if join was successful, 0 if it wasn't
     */
    int joinGroup(Group* group, int socket_id);

    /**
     * Tries to leave the given group
     * @param group Instance of the group the user wishes to leave
     * @param socket_id Identifier for the socket corresponding to the finishing thread
     * @returns 1 if that was the last user in the group, 0 otherwise
     */
    int leaveGroup(Group* group, int socket_id);

    /**
     * Sends a message said by this user to the group
     * @param message   Chat message this user wants to say
     * @param groupname Name of the group this message is being sent to
     * @return Number of users this message was sent to, should always be at least 1 (the sender) on success
     */
    int say(std::string message, std::string groupname);


    /**
     * Signals the user instance that a new message has arrived to the group
     * The user instance then signals the corresponding client threads to send packets
     * @param message Text message that will be sent to the clients
     * @param username Username of the user who posted this message
     * @param groupname Groupname of the group where the message was posted
     * @param packet_type Packet type
     * @returns 
     */
    int signalNewMessage(std::string message, std::string username, std::string groupname, int packet_type);
};

#endif