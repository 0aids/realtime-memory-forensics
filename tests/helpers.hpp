#pragma once
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <sys/prctl.h>
#include <sys/signal.h>
#include <csignal>
#include <thread>
#include "rmf/memory_region.hpp"

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

template <rmf::Numeric N>
static auto makeNumData(std::initializer_list<N> vals) -> std::vector<uint8_t>
{
    std::vector<uint8_t> data;
    data.reserve(vals.size() * sizeof(N));
    for (auto v : vals)
    {
        auto* p = reinterpret_cast<const uint8_t*>(&v);
        for (size_t i = 0; i < sizeof(N); ++i)
            data.push_back(p[i]);
    }
    return data;
}

static auto makeMRV(std::vector<uint8_t> data, uintptr_t pAddr = 0x1000,
                    ptrdiff_t rAddr = 0) -> rmf::MemoryRegion
{
    rmf::Map map{
        .name  = std::make_shared<const std::string>("test"),
        .pAddr = pAddr,
        .pSize = static_cast<uintptr_t>(data.size()),
        .rAddr = rAddr,
        .rSize = static_cast<ptrdiff_t>(data.size()),
    };
    rmf::Snapshot snap{
        .data = std::make_shared<std::vector<uint8_t>>(std::move(data)),
    };
    return {.map = std::move(map), .snap = std::move(snap)};
}
