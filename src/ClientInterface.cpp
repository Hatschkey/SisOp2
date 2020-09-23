#include "ClientInterface.h"

std::string ClientInterface::groupname;

WINDOW* ClientInterface::infoscr;
WINDOW* ClientInterface::chatscr;
WINDOW* ClientInterface::inptscr;

void ClientInterface::init(std::string group)
{
    // Initialize group name
    ClientInterface::groupname = group;

    initscr();  

    // Initialize top, middle and bottom segment
    infoscr = newwin(        2, COLS - 1,         0, 0);
    chatscr = newwin(LINES - 5, COLS - 1,         2, 0); // Leave 2 lines for top and bottom segments
    inptscr = newwin(        2, COLS - 1, LINES - 2, 0);

    // Clear these screens to start with a blank canvas
    wclear(infoscr);
    wclear(chatscr);
    wclear(inptscr);

    // Enable scroll on chat screen
    scrollok(chatscr, true);

    // Print the UI components
    ClientInterface::printUI();
}

void ClientInterface::destroy()
{
    // Delete all windows
    delwin(infoscr);
    delwin(chatscr);
    delwin(inptscr);

    refresh();
    endwin();
}

void ClientInterface::printMessage(std::string message)
{
    //int message_line_count = (int)ceil((float)message.length()/getmaxx(chatscr)); // Calculate amount of lines this message occupies

    // Get current x and y for cursor
    //int cursor_y = getcury(chatscr);

    // Print message to screen
    wprintw(chatscr, (message + "\n").c_str());

    //wmove(chatscr, cursor_y + message_line_count, 0);
/*
    // Check if printing the message would exceed max screen size
    if (cursor_y + message_line_count >= getmaxy(chatscr) - 1)
    {

    }
    else
    {
        // If not, just print the message
        waddstr(chatscr, message.c_str());
        
        // Move the cursor to the next line
        wmove(chatscr, cursor_y + message_line_count, 0);
    }
*/

    // Refresh screen
    wrefresh(chatscr);
}

void ClientInterface::printUI()
{
    std::string top_ui = "Connected to group: " + groupname;
    std::string bottom_ui = "Say something: ";

    // Add UI at the top
    mvwaddstr(infoscr, 0, 0, top_ui.c_str());

    // Add UI at the bottom
    mvwaddstr(inptscr, 0, 0, bottom_ui.c_str());

    // Refresh screens
    wrefresh(infoscr);
    wrefresh(inptscr);
}

void ClientInterface::resetInput()
{

    // Clear user message
    wclear(inptscr);

    // Reprint the UI
    ClientInterface::printUI();

    // Refresh screen
    wrefresh(inptscr);
}
