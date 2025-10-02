#include "log.hpp"

int main() {
    Log(Debug, "This is a test");
    Log(Error, "This is a test");
    Log(Message, "This is a test");
    Log(Warning, "This is a test");
    Log(Verbose, "This is a test");

    return 0;
}
