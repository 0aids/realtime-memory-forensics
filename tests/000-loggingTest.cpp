#include "utils/logs.hpp"
#include "tests.hpp"

int main() {

    rmf_Log(rmf_Debug, "Debug test!");
    rmf_Log(rmf_Verbose, "Verbose Test!");
    rmf_Log(rmf_Message, "Message test!");
    rmf_Log(rmf_Warning, "Warning test!");
    rmf_Log(rmf_Error, "Error test!");

    rmf::utils::Logger::currentLevel = rmf_Error;

    rmf_Log(rmf_Warning, "You shouldn't be able to see this!!!");
    rmf_Log(rmf_Error, "You should be able to see this!!!");

    return 0;
}
