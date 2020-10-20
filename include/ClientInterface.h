#ifndef CLIENTINTERFACE_H
#define CLIENTINTERFACE_H

#include <curses.h>
#include <string>
#include <cmath>
#include <csignal>
#include <unistd.h>
#include <chrono>


class ClientInterface
{
public:
     static WINDOW *inptscr; // Bottom segment of the terminal where the user can type outgoing messages

private:
     static WINDOW *infoscr; // Top segment of the terminal, presents info about the current session
     static WINDOW *chatscr; // Middle segment of the terminal where incoming chat messages are shown

     static std::string groupname; // Name of the group user is connected to
     static std::string username;  // Name of the user connected

public:
     /**
      * @brief Initialize client interface using the ncurses library
      */
     static void init(std::string group, std::string user);

     /**
      * @brief End client interface and free ncurses resources
      */
     static void destroy();

     /**
      * @brief Print a new message to the screen
      */
     static void printMessage(std::string message);

     /**
      * @brief Prints session at the top of the screen
      */
     static void printUI();

     /**
      * @brief Resets the user input area below the screen
      * Deletes the old message and resets the cursor position
      */
     static void resetInput();

     /**
      * @brief Updates the clock on the UI 
      */
     static void *updateUI(void* arg);

     /**
      * @brief Handles a terminal resize signal 
      */
     static void handleResize(int signum);
};

#endif