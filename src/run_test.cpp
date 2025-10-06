#include "run_test.hpp"
#include <list>
#include <chrono>
#include <fstream>
#include <ratio>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include "log.hpp"
#include <cstdlib>
#include <unistd.h>

__attribute__((optimize("O0"))) void changingMapMemorySampleProcess()
{
    using namespace std;
    string thing;
    thing = "this is a thing!!!";
    this_thread::sleep_for(1000ms);
    while (true)
    {
        thing.reserve(10000000);
        this_thread::sleep_for(1000ms);
        thing.shrink_to_fit();
    }
}

pid_t runChangingMapProcess()
{
    //
    pid_t pid = fork();
    if (pid < 0)
    {
        Log(Error, "Forking failed");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        changingMapMemorySampleProcess();
        _exit(127);
    }
    return pid;
}

__attribute__((optimize("O0"))) void sampleProcess()
{
    using namespace std;

    char          extrashit[] = "This is some extra shit";
    volatile char i[]         = "ALL HAIL THE!!!";
    volatile char j[]         = "A FASTER!!!";
    string*       randomthing = new string("small string");
    double*       doubleVal   = new double(150);
    auto          list = new std::list<char>{'a', 'b', 'c', 'd'};
    cerr << "randomthing address: " << std::hex << std::showbase
         << &randomthing << endl;
    cerr << "linked list address: " << std::hex << std::showbase
         << &list << endl;

    string* otherrandomshit = new string(
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Mauris "
        "commodo, felis non semper fermentum, lorem velit dictum "
        "risus, id "
        "rutrum ipsum nisi eget ex. Curabitur ullamcorper "
        "consectetur venenatis. "
        "Vestibulum ultrices iaculis arcu quis suscipit. Duis "
        "tincidunt "
        "venenatis fermentum. Fusce vehicula consequat erat, quis "
        "bibendum arcu "
        "iaculis a. In pharetra ligula massa. Pellentesque nisi ex, "
        "vehicula at "
        "est nec, molestie egestas enim. Integer eleifend eros in "
        "leo tempor, at "
        "gravida lectus hendrerit. Aenean at ligula eu leo "
        "pellentesque "
        "elementum. Morbi sed mattis metus, sit amet ultrices orci. "
        "Sed euismod, "
        "ex sed hendrerit sagittis, nibh sapien lobortis nisl, ut "
        "dignissim "
        "tellus nulla ut ex. Phasellus eu convallis odio. Morbi "
        "dignissim "
        "bibendum lorem, quis sollicitudin est vehicula facilisis. "
        "Cras "
        "venenatis in enim vitae efficitur. Sed lacinia viverra "
        "turpis elementum "
        "maximus. Nam sit amet rhoncus nisl. Etiam dapibus ligula "
        "vel faucibus "
        "dapibus. Curabitur commodo ante non arcu efficitur "
        "hendrerit. Ut "
        "consectetur scelerisque nulla, id molestie metus commodo "
        "ac. Etiam a "
        "efficitur orci, ac bibendum velit. In lacinia viverra ex id "
        "euismod. "
        "Sed nec dolor vel purus rutrum pharetra ac non est. Nullam "
        "lobortis "
        "rutrum ligula, nec sagittis sem bibendum nec. Vivamus "
        "sagittis ligula "
        "enim, et tempus ex porttitor in. Cras mattis ex in nibh "
        "tempus, non "
        "rhoncus risus iaculis. Sed tincidunt elementum ligula in "
        "convallis. Nam "
        "faucibus ipsum in sagittis dignissim. Nam a elit mauris. "
        "Pellentesque "
        "auctor erat sed pellentesque tempor. Etiam at congue ante. "
        "Pellentesque ");
    size_t iterations = 0;
    while (true)
    {
        iterations++;
        this_thread::sleep_for(1ms);
        if (!(iterations % 500))
        {
            i[0] = (i[0] - 64) % 26 + 65;
        }
        j[0] = (i[0] - 64) % 26 + 65;
    }
    delete randomthing;
    delete otherrandomshit;
}

bool checkPtraceScope()
{
    const std::string_view ptrace_scope =
        "/proc/sys/kernel/yama/ptrace_scope";
    std::ifstream inputFile(ptrace_scope.data());
    std::string   line;
    if (!std::getline(inputFile, line))
    {
        Log(Error, "Failed to open '" << ptrace_scope << "'");
        return false;
    }
    if (line != "0")
    {
        Log(Error,
            "File: '" << ptrace_scope
                      << "' is not 0, unable to continue");
        return false;
    }
    return true;
}

pid_t runSampleProcess()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        Log(Error, "Forking failed");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        sampleProcess();
        _exit(127);
    }
    return pid;
}
