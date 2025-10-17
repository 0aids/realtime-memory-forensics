#include "logs.hpp"
#include "tests.hpp"

int main() {

    Log(Debug, "Debug test!");
    Log(Verbose, "Verbose Test!");
    Log(Message, "Message test!");
    Log(Warning, "Warning test!");
    Log(Error, "Error test!");

    Logger::currentLevel = Error;

    Log(Warning, "You shouldn't be able to see this!!!");
    Log(Error, "You should be able to see this!!!");

    return 0;
}
