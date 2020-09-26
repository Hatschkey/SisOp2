#include "Group.h"

std::map<std::string, Group*> Group::active_groups;
RW_Monitor Group::active_groups_monitor;

Group::Group(std::string groupname)
{
    long message_count; // Counter for how many messages are in the group history file

    // Update groupname
    this->groupname = groupname;

    // Name of the group's history file
    std::string history_filename = groupname.append(".hist");

    // Request write rights
    history_file_monitor.requestWrite();

    // Check if it already existed
    if ( (this->history_file = fopen((HIST_PATH + history_filename).c_str(),"rb+")) == NULL)
    {
        // If not, create it
        this->history_file = fopen((HIST_PATH + history_filename).c_str(),"wb+");

        // And add the header into it
        message_count = 0;
        fwrite(&message_count,sizeof(long), 1, this->history_file);
    }

    // Flush file
    fflush(this->history_file);

    // Release write rights
    history_file_monitor.releaseWrite();

    // Add itself to group list
    Group::addGroup(this);
}

Group::~Group()
{
    // Request write rights
    history_file_monitor.releaseWrite();

    // Close group history file
    fclose(this->history_file);

    // Release write rights
    history_file_monitor.releaseWrite();
}

Group* Group::getGroup(std::string groupname)
{
    Group* group; // Reference to the group
    int exists = 0;

    try
    {
        // Try to find groupname in map
        group = active_groups.at(groupname);

        exists = 1;
    }
    catch(const std::out_of_range& e)
    {
        // If reached end of map, group is not there
        exists = 0;
    }

    return exists? group : new Group(groupname);
}

void Group::addGroup(Group* group)
{
    // Insert in group map
    active_groups.insert(std::make_pair(group->groupname, group));
}

int Group::removeGroup(std::string groupname)
{
    int removed_groups = 0; // Number of removed groups

    // Request write rights
    Group::active_groups_monitor.requestWrite();

    // Remove group from map
    removed_groups = active_groups.erase(groupname);

    // Release write rights
    Group::active_groups_monitor.releaseWrite();

    return removed_groups;
}

void Group::listGroups()
{
    // Request read rights
    Group::active_groups_monitor.requestRead();

    // Delimiter
    std::cout << "======================\n" << std::endl;
    
    // Iterate map listing groups and their users
    for (std::map<std::string,Group*>::iterator i = active_groups.begin(); i != active_groups.end(); ++i)
    {
        std::cout << "Groupname: " << i->second->groupname << std::endl;
        std::cout << "User count: " << i->second->getUserCount() << std::endl;
        std::cout << "Users: " << std::endl;
        // List all users in the group
        i->second->listUsers();
        std::cout << std::endl;
    }

    // Delimiter
    std::cout << "======================" << std::endl;

    // Release read rights
    Group::active_groups_monitor.releaseRead();
}

int Group::joinByName(std::string username, std::string groupname, User** user, Group** group, int socket_id)
{
    int status = 0; // Status indicating if the user was able to join the group

    // Request read rights
    Group::active_groups_monitor.requestWrite();
    User::active_users_monitor.requestWrite();

    // Get user and group reference
    *user = User::getUser(username);
    *group = Group::getGroup(groupname);

    // Try to join the group with that user
    status = (*user)->joinGroup(*group, socket_id);

    // Release read rights
    Group::active_groups_monitor.releaseWrite();
    User::active_users_monitor.releaseWrite();

    // Return join status
    return status;
}

void Group::addUser(User* user)
{
    // Request write rights
    this->users_monitor.requestWrite();

    // Insert user in map
    users.insert(std::make_pair(user->username, user));

    // Release write rights
    this->users_monitor.releaseWrite();
}

int Group::removeUser(std::string username)
{
    // Request write rights
    this->users_monitor.requestWrite();

    // Erase user from vector
    users.erase(username);

    // Release write rights
    this->users_monitor.releaseWrite();

    // Check how many users are left in the group
    if (this->getUserCount() == 0)
    {
        // If no users are left, remove itself from the static list

        // Request write rights
        Group::active_groups_monitor.requestWrite();

        // Remove itself from the static list
        Group::active_groups.erase(this->groupname);

        // Release write rights
        Group::active_groups_monitor.releaseWrite();

        // And delete itself
        delete(this);

        return 1;
    }

    return 0;
}

void Group::listUsers()
{
    // Request read rights
    this->users_monitor.requestRead();

    // Iterate vector listing all users
    for (std::map<std::string, User*>::iterator i = users.begin(); i != users.end(); ++i)
        std::cout << " - User: " << i->second->username << std::endl;

    // Release read rights
    this->users_monitor.releaseRead();
}

int Group::getUserCount()
{
    int user_count = 0; // Current connected user count

    // Request read rights
    this->users_monitor.requestRead();

    user_count = users.size();

    // Release read rights
    this->users_monitor.releaseRead();

    return user_count;
}

int Group::post(std::string message, std::string username)
{
    int sent_messages = 0; // Number of messages that were sent

    // Save this message
    this->saveMessage(message, username, USER_MESSAGE);

    // Request read rights
    users_monitor.requestRead();

    // Send message to every connected user (Including message sender)
    for (std::map<std::string, User*>::iterator i = users.begin(); i != users.end(); ++i)
    {
        // Signal each user instance in the group that a new message was posted
        sent_messages += i->second->signalNewMessage(message, username, this->groupname, PAK_DATA, USER_MESSAGE);
    }

    // Release read rights
    users_monitor.releaseRead();

    // Return the amount of messages that were issued
    return sent_messages;
}

void Group::saveMessage(std::string message, std::string username, int message_type)
{
    int record_size = -1;   // Size of the message that will be sent
    long message_count = -1; // Number of messages already present in the file

    // Compose the message
    message_record* msg = CommunicationUtils::composeMessage(username, message, message_type);

    // Calculate message size
    record_size = sizeof(*msg) + msg->length;

    // Request writing rights
    history_file_monitor.requestWrite();

    // Write message at the end of group history file
    fseek(this->history_file, 0, SEEK_END);
    fwrite(msg, record_size, 1, this->history_file);

    // Update file header to increase message count
    fseek(this->history_file, 0, SEEK_SET);
    fread(&message_count, sizeof(long), 1, history_file);
    message_count++;
    fseek(this->history_file, 0, SEEK_SET);
    fwrite(&message_count, sizeof(long), 1, history_file);

    // Move pointer back to the end of the file
    fseek(this->history_file, 0, SEEK_END);

    // Update file
    fflush(this->history_file);

    // Release writing rights
    history_file_monitor.releaseWrite();

    // Free memory used for message record
    free(msg);
}

int Group::recoverHistory(char* message_record_list, int n, User* user)
{
    char header_buffer[PACKET_MAX]; // Buffer for message headers that will be read
    char message_buffer[PACKET_MAX]; // Buffer for messages that will be read
    long total_messages = 0;  // Total number of messages in the file
    long current_message = 0; // Currently indexed message in the file
    int read_messages = 0; // Number of messages that were read and sent to user
    int offset = 0; // Number of bytes needed to shift after the insert of a message into the message_record_list

    // Request reading rights
    history_file_monitor.requestRead();

    // Open history file for reading
    FILE* hist = fopen((HIST_PATH + std::string(this->groupname + ".hist")).c_str(), "rb");

    // Get total message count
    fread(&total_messages, sizeof(long), 1, hist);

    // Iterate through history file reading headers
    while (current_message < total_messages && fread(header_buffer, sizeof(message_record), 1, hist) > 0)
    {
        // If the message is in the last N
        if (current_message >= total_messages - n)
        {

            // Read the associated message
            fread(message_buffer,((message_record*)header_buffer)->length, 1, hist);

            // Copy the message header into the buffer
            memcpy(message_record_list + offset, header_buffer, sizeof(message_record));

            // Update the offset
            offset += sizeof(message_record);

            // Copy the message payload into the buffer
            memcpy(message_record_list + offset, message_buffer, ((message_record*)header_buffer)->length);

            // Update the offset
            offset += ((message_record*)header_buffer)->length;

            // Increase read message counter
            read_messages++;
        }
        else
        {
            // If not, only jump to the next message
            fseek(hist, ((message_record*)header_buffer)->length,SEEK_CUR);
        }

        // Increase current message
        current_message++;

        // Reset buffers for reading new messages
        bzero(header_buffer, PACKET_MAX);
        bzero(message_buffer, PACKET_MAX);
    }

    // Close file that was opened for reading
    fclose(hist);

    // Release reading rights
    history_file_monitor.releaseRead();

    // Return read messages
    return read_messages;
}

int Group::broadcastMessage(std::string message, std::string username)
{
    // Save message
    this->saveMessage(message, username, SERVER_MESSAGE);

    this->users_monitor.requestRead();

    // Send login/logout message to every connected users
    for (std::map<std::string, User*>::iterator i = users.begin(); i != users.end(); ++i)
    {
        i->second->signalNewMessage(message, username, this->groupname, PAK_SERVER_MESSAGE, SERVER_MESSAGE);
    }

    this->users_monitor.releaseRead();

    return 0;
}