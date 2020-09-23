#ifndef CLIENTINTERFACE_H
#define CLIENTINTERFACE_H

#include <curses.h>
#include <string>
#include <cmath>
#include <csignal>

class ClientInterface
{
    public:

        static WINDOW* inptscr; // Bottom segment of the terminal where the user can type outgoing messages

    private:

        static WINDOW* infoscr; // Top segment of the terminal, presents info about the current session
        static WINDOW* chatscr; // Middle segment of the terminal where incoming chat messages are shown

        static std::string groupname; // Name of the group user is connected to

    public:
        /**
         * Initialize client interface using the ncurses library
         */
        static void init(std::string group);

        /**
         * End client interface and free ncurses resources
         */
        static void destroy();

        /**
         * Print a new message to the screen
         */
        static void printMessage(std::string message);

        /**
         * Prints groupname and date on top of the screen, as well as user input at the bottom
         */
        static void printUI();

        /**
         * Resets the user input area below the screen
         * Deletes the old message and resets the cursor position
         */
        static void resetInput();

        /**
         * Handles a terminal resize signal 
         */
        static void handleResize(int signum);
};

#endif