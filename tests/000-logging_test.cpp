#include "logger.hpp"

int main() {
    rmf_Log(rmf_Debug, "This is a debug test");
    rmf_Log(rmf_Verbose, "This is a verbose test");
    rmf_Log(rmf_Info, "This is a info test");
    rmf_Log(rmf_Warning, "This is a warning test");
    rmf_Log(rmf_Error, "This is a error test");
}
