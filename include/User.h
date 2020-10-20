#ifndef USER_H
#define USER_H

#include <string>
#include <chrono>
#include <vector>
#include <pthread.h>

#include "constants.h"
#include "data_types.h"
#include "Group.h"
#include "Session.h"
#include "RW_Monitor.h"
#include "CommunicationUtils.h"

// Forward declare Group and session
class Group;
class Session;

class User : protected CommunicationUtils
{
public:
    static std::map<std::string, User *> active_users; // Current active users
    static RW_Monitor active_users_monitor;            // Monitor for active users

    std::string username; // User display name
    uint64_t last_seen;   // Last time a message was received from this user

    std::map<int, Session *> sessions; // User active sessions
    RW_Monitor session_monitor;        // Monitor for active session list

    /**
     * Searches for the given username in the currently active users list.
     * TODO This method could be read-write protected, in order to allow only 1 writer at a time or multiple readers at a time
     * @param username Username of the user to search for
     * @return Reference to the user structure in the user list
     */
    static User *getUser(std::string username);

    /**
     * Add user to currently active user list
     * TODO This method also should be read-write protected, only one user may be added at a time
     * @param user Pointer to user that will be added to list
     */
    static void addUser(User *user);

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
     * Returns the current session count for this user instance in the specified group
     * @param groupname Name of the group
     * @return Amount of active sessions in that group
     */
    int getSessionCount(std::string groupname);

    /**
     * Tries to join the given group, if the session count allows for it
     * @param group   Instance of a Group class
     * @param session Instance of the session for this connection
     * @return 1 if join was successful, 0 if it wasn't
     */
    int joinGroup(Group *group, Session *session);

    /**
     * Tries to leave the given group
     * @param session Instace of the session for the connection being closed
     * @returns 1 if that was the last user in the group, 0 otherwise
     */
    int leaveGroup(Session *session);

    /**
     * Signals the user instance that a new message has arrived to the group
     * The user instance then signals the corresponding client threads to send packets
     * @param message Text message that will be sent to the clients
     * @param username Username of the user who posted this message
     * @param groupname Groupname of the group where the message was posted
     * @param packet_type Packet type
     * @param message_type Indicates wether the message type is USER_MESSAGE or SERVER_MESSAGE
     * @returns 
     */
    int signalNewMessage(std::string message, std::string username, std::string groupname, int packet_type, int message_type);

    /**
     * Updates the user's last seen attribute to current time
     */
    void setLastSeen();
};

#endif