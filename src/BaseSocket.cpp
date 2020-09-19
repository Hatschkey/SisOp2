#include "BaseSocket.h"

std::string BaseSocket::appendErrorMessage(const std::string message)
{
    return message + " (" + std::string(strerror(errno)) + ")";
}