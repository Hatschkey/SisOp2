#include <curses.h>
#include <string>
#include <cmath>

class ClientInterface
{

    private:
    
        static std::string groupname; // Name of the group user is connected to

        static int max_lines;         // Line limit
        static int max_columns;       // Column limit
        static int last_message_end;  // Line where the last message ended

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
};