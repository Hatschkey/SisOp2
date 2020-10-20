#include "ClientInterface.h"

std::string ClientInterface::groupname;
std::string ClientInterface::username;

WINDOW *ClientInterface::infoscr;
WINDOW *ClientInterface::chatscr;
WINDOW *ClientInterface::inptscr;

void ClientInterface::init(std::string group, std::string user)
{
    // Initialize group name
    ClientInterface::groupname = group;
    ClientInterface::username = user;

    initscr();

    // Initialize top, middle and bottom segment
    infoscr = newwin(3, COLS - 1, 0, 0);
    chatscr = newwin(LINES - 6, COLS - 1, 3, 0);
    inptscr = newwin(2, COLS - 1, LINES - 2, 0);

    // Clear these screens to start with a blank canvas
    wclear(infoscr);
    wclear(chatscr);
    wclear(inptscr);

    // Enable scroll on chat screen
    scrollok(chatscr, true);

    // Print the UI components
    ClientInterface::printUI();

    std::signal(SIGWINCH, ClientInterface::handleResize);
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
    // Print message to screen
    wprintw(chatscr, (message + "\n").c_str());

    // Refresh screen
    wrefresh(chatscr);
}

void ClientInterface::printUI()
{
    std::string bottom_ui = "Say something: ";

    // Add UI at the bottom
    mvwaddstr(inptscr, 0, 0, bottom_ui.c_str());

    // Refresh screen
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

void ClientInterface::handleResize(int signum)
{
    // Resize windows according to new terminal size
    wresize(infoscr, 2, COLS - 1);
    wresize(chatscr, LINES - 5, COLS - 1);
    wresize(inptscr, 2, COLS - 1);

    // Refresh windows
    wrefresh(infoscr);
    wrefresh(chatscr);
    wrefresh(inptscr);
}

void *ClientInterface::updateUI(void *arg)
{
    // Current time variables
    time_t now;
    char time_string[9];

    std::string top_ui;

    while (true)
    {
        // Sleep for a second
        sleep(1);

        // Add group/user string
        top_ui = "Connected as " + username + " to group: " + groupname;
        mvwaddstr(infoscr, 0, 0, top_ui.c_str());

        // Current time
        now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        // Get time into a readable format
        strftime(time_string, sizeof(time_string), "%H:%M:%S", std::localtime(&now));

        // Add time string
        top_ui = "It is currently " + std::string(time_string);
        mvwaddstr(infoscr, 1, 0, top_ui.c_str());

        // Refresh screen
        wrefresh(infoscr);
    }
}
