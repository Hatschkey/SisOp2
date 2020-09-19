#include <string>
#include <cstring>
#include <errno.h>

class BaseSocket {
    protected:
    // Protected methods
    static std::string appendErrorMessage(const std::string message); // Add details of the error to the message
};