#include "ClientInterface.h"

std::string ClientInterface::groupname;

int ClientInterface::max_lines;
int ClientInterface::max_columns;
int ClientInterface::last_message_end;

void ClientInterface::init(std::string group)
{
    // Initialize group name
    ClientInterface::groupname = group;

    // Initialize standard screen
    initscr();

    // Clear screen to start with a blank canvas
    clear();

    // Initialize max lines and columns, as well as last message end
    max_lines        = LINES - 1;
    max_columns      = COLS - 1;
    last_message_end = 2;

    // Print the UI components
    ClientInterface::printUI();
}

void ClientInterface::destroy()
{
    refresh();
    endwin();
}

void ClientInterface::printMessage(std::string message)
{
    int message_line_count = 1 + (int)ceil((float)message.length()/max_columns); // Calculate amount of lines this message occupies

    // Save old x and y cursor pos
    int old_cursor_x;
    int old_cursor_y;
    getyx(stdscr,old_cursor_y,old_cursor_x);

    // Check if last message was too close to the bottom of the screen
    if (ClientInterface::last_message_end + message_line_count >= max_lines - 2)
    {
        // If so, clear the screen to make space for it
        // TODO Maybe try to do some sort of scroll afterwards?
    }
    else
    {
        // If not, just print the message to the screen
        mvaddstr(ClientInterface::last_message_end, 0, message.c_str());
    }

    // Refresh screen
    refresh();

    // Increase last message end
    ClientInterface::last_message_end += message_line_count;

    // Move the cursor back to the bottom
    move(old_cursor_y,old_cursor_x);

    // Refresh screen
    refresh();
}

void ClientInterface::printUI()
{
    std::string top_ui = "Connected to group: " + groupname;
    std::string bottom_ui = "Say something: ";

    // Add UI at the top
    mvaddstr(0,0,top_ui.c_str());

    // Add UI at the bottom
    mvaddstr(max_lines, 0, bottom_ui.c_str());

    // Refresh screen
    refresh();
}

void ClientInterface::resetInput()
{
    // Move cursor to bottom
    move(max_lines, 15);

    // Clear user message
    clrtoeol();

    // Move back to start
    move(max_lines, 15);

    refresh();
}