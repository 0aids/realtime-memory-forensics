#include "test_helpers.hpp"
#include "logger.hpp"
#include <cstdlib>
#include <csignal>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <list>

namespace rmf::test {

#pragma GCC push_options
#pragma GCC optimize("O0")

static void testFunction()
{
    using namespace std;

    volatile char    extrashit[] = "This is some extra shit";
    volatile char    i[]         = "ALL HAIL THE!!!";
    volatile char    j[]         = "A FASTER!!!";
    volatile double* d           = new double(5);
    volatile string* randomthing = new string("small string");
    volatile double* doubleVal   = new double(150);
    volatile auto    list = new std::list<char>{'a', 'b', 'c', 'd'};
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
    size_t           iterations = 0;
    volatile size_t* dumb       = new size_t(0);
    while (true)
    {
        iterations++;
        this_thread::sleep_for(1ms);
        if (!(iterations % 500))
        {
            i[0] = (i[0] - 64) % 26 + 65;
        }
        j[0] = (i[0] - 64) % 26 + 65;
        *d += 0.5;
        *dumb += 1;
    }
    delete randomthing;
    delete otherrandomshit;
}

#pragma GCC pop_options

pid_t startTestFunction() {
    pid_t pid = fork();
    if (pid < 0)
    {
        rmf_Log(rmf_Error, "Forking failed");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1)
        {
            perror("prctl failed");
            exit(EXIT_FAILURE);
        }
        int ret =
            setpriority(PRIO_PROCESS, 0,
                        -20); // higher priority (lower nice value)
        if (!ret)
        {
            rmf_Log(rmf_Warning, "Failed to change forked process priority!");
        }
        testFunction();
        _exit(127);
    }
    rmf_Log(rmf_Debug, "Child pid: " << pid);
    return pid;
}
};
