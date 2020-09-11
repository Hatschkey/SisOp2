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
 
    // Check if it already existed
    if ( (this->history_file = fopen(history_filename.c_str(),"rb+")) == NULL)
    {
        // If not, create it
        this->history_file = fopen(history_filename.c_str(),"wb+");

        // And add the header into it
        message_count = 0;
        fwrite(&message_count,sizeof(long), 1, this->history_file);
    }

    // Flush file
    fflush(this->history_file);

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
    Group* group; // Reference to the group

    try
    {
        // Request read rights
        Group::active_groups_monitor.requestRead();

        // Try to find groupname in map
        group = active_groups.at(groupname);

    }
    catch(const std::out_of_range& e)
    {
        // If reached end of map, group is not there
        group = NULL;
    }
    
    // Release read rights
    Group::active_groups_monitor.releaseRead();

    return group;
}

void Group::addGroup(Group* group)
{
    // Request write rights
    Group::active_groups_monitor.requestWrite();

    // Insert in group map
    active_groups.insert(std::make_pair(group->groupname, group));
    
    // Release the write rights
    Group::active_groups_monitor.releaseWrite();

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

    // Iterate map listing groups and their users
    for (std::map<std::string,Group*>::iterator i = active_groups.begin(); i != active_groups.end(); ++i)
    {
        std::cout << std::endl;
        std::cout << "Groupname: " << i->second->groupname << std::endl;
        std::cout << "User count: " << i->second->getUserCount() << std::endl;
        // List all users in the group
        i->second->listUsers();
    }

    // Release read rights
    Group::active_groups_monitor.releaseRead();
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
    int removed_users = 0; // Amount of users that was removed

    // Request write rights
    this->users_monitor.requestWrite();

    // Erase user from vector
    removed_users = users.erase(username);

    // Release write rights
    this->users_monitor.releaseWrite();

    // Check how many users are left in the group
    if (this->users.size() == 0)
    {
        // If no users are left, remove itself from the static list

        // Request write rights
        Group::active_groups_monitor.requestWrite();

        // Remove itself from the static list
        Group::active_groups.erase(this->groupname);

        // Release write rights
        Group::active_groups_monitor.releaseWrite();

        // Call desctructor
        this->~Group();

        // And delete itself
        free(this);
    }

    return removed_users;
}

void Group::listUsers()
{

    // Request read rights
    this->users_monitor.requestRead();

    // Iterate vector listing all users
    for (std::map<std::string, User*>::iterator i = users.begin(); i != users.end(); ++i)
        std::cout << "User: " << i->second->username << std::endl;

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

    // Save this message
    this->saveMessage(message, username);

    // Send message to every connected user (Including message sender)
    for (std::map<std::string, User*>::iterator i = users.begin(); i != users.end(); ++i)
    {
        // TODO Placeholder debug message
        std::cout << "TODO Send message to " << i->second->username << std::endl;
    }

    // TODO Change here to return the amount of messages that were issued
    return users.size();
}

void Group::saveMessage(std::string message, std::string username)
{
    // Create a record for the user message
    message_record* msg = (message_record*)malloc(sizeof(message_record));
    sprintf(msg->username,"%s", username.c_str());  // Copy username
    msg->timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current timestamp
    msg->length = strlen(message.c_str()) + 1; // Update message length
    sprintf((char*)msg->_message,"%s",message.c_str()); // Copy message

    // Calculate struct size
    int record_size = sizeof(*msg) + sizeof(char) * (strlen(message.c_str()) + 1);

    // TODO Mutex protect this writing process
    // Write message at the end of group history file
    fseek(this->history_file, 0, SEEK_END);
    fwrite(msg, record_size, 1, this->history_file);

    // Update file header to increase message count
    long message_counter = 0;
    fseek(this->history_file, 0, SEEK_SET);
    fread(&message_counter,sizeof(long), 1,history_file);
    message_counter++;
    fseek(this->history_file, 0, SEEK_SET);
    fwrite(&message_counter, sizeof(message_counter), 1, history_file);

    // Move pointer back to the end of the file
    fseek(this->history_file, 0, SEEK_END);

    // Update file
    fflush(this->history_file);

    // Free memory used for message record
    free(msg);
}

int Group::recoverHistory(int n, User* user)
{
    char header_buffer[PACKET_MAX]; // Buffer for message headers that will be read
    char message_buffer[PACKET_MAX]; // Buffer for messages that will be read
    long total_messages = 0;  // Total number of messages in the file
    long current_message = 0; // Currently indexed message in the file
    int read_messages = 0; // Number of messages that were read and sent to user

    // Open history file for reading
    FILE* hist = fopen(std::string(this->groupname + ".hist").c_str(), "rb");

    // Get total message count
    fread(&total_messages, sizeof(long), 1, hist);

    // TODO Debug message
    std::cout << "Group history file has " << total_messages << " messages" << std::endl;

    // Iterate through history file reading headers
    while (current_message < total_messages && fread(header_buffer, sizeof(message_record), 1, hist) > 0)
    {
        // If the message is in the last N
        if (current_message >= total_messages - n)
        {
            // Read the associated message
            fread(message_buffer,((message_record*)header_buffer)->length, 1, hist);

            // TODO Placeholder send message to user
            std::cout << "TODO Send message \"" << message_buffer << "\" by " << ((message_record*)header_buffer)->username << " to user " << user->username << std::endl;

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
    }

    // Close file that was opened for reading
    fclose(hist);

    // Return read messages
    return read_messages;
}

