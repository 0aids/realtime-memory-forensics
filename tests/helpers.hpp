#pragma once
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <sys/prctl.h>
#include <sys/signal.h>
#include <csignal>
#include <thread>

struct ForkedProcess
{
    pid_t pid = 0;

  public:
    ForkedProcess(auto&& func)
    {
        pid = fork();
        if (pid < 0)
            throw std::runtime_error("Failed to fork process!");

        else if (pid > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return;
        }

        if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1)
        {
            perror("prctl failed");
            exit(EXIT_FAILURE);
        }
        func();
        exit(127);
    }
    ~ForkedProcess()
    {
        kill(pid, SIGTERM);
    }
};

static void infiniteLoopFunc()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(100));
    }
}
