#include "rmf/op.hpp"
#include "rmf/utils/meta.hpp"
#include "rmf/utils/threadpool.hpp"
#include <cstring>
#include <exception>
#include <print>
#include <rmf/rmf.hpp>
#include <rmf/snapshot.hpp>
#include <rmf/utils/vec.hpp>
#include <stdexcept>
#include <string>
namespace mf  = rmf;
namespace mfu = mf::Utils;
using namespace std;

void printHelp(const char* self)
{
    println("Usage: {} PID String", self);
    println("Prints the first 10 bytes before the beginning "
            "of inputted string.");
}

int main(int argc, const char* argv[])
{
    if (argc < 3)
    {
        println("Missing inputs: PID String");
        printHelp(argv[0]);
        return 1;
    }
    for (int i = 1; i < argc; i++)
    {
        if (strcmp("-h", argv[i]) == 0)
        {
            printHelp(argv[0]);
            return 1;
        }
    }
    pid_t PID;
    try
    {
        PID = std::stoull(argv[1]);
    }
    catch (const std::exception& e)
    {
        println("Invalid PID! {}", e.what());
        printHelp(argv[0]);
        return 1;
    }
    const std::string strToFind = argv[2];

    using namespace mfu;
    using namespace mf;
    ThreadPool tp(thread::hardware_concurrency() / 2);
    // Read the maps, only get readable.
    Vec<Node<Map, Snapshot>> maps = getMaps<Snapshot>(PID).hasPerms("r");

    // Capture everything
    // captureNodes.threaded(maps, PID).with(tp);
    // maps.map(Snapshot::captureM, PID);
    Snapshot::captureM(maps[0], PID);
    // maps.mapThreaded<Snapshot::captureM>(PID).with(tp);
    maps.pipe() | Snapshot::captureF(PID) // | Snapshot::captureF(PID)
        | Pipe::End{};

    // Search for the string
    auto newMaps = maps.pipe() | findStringF(strToFind) | Pipe::End{};

    // Will have to implement my own piping so consolidation can happen automatically.
    for (auto& map : consolidate(newMaps))
    {
        println("{}", string(map));
        map.map.relativeAddress -= 10;
        map.map.relativeSize += 10;
        map.capture(PID);
        println("Bytes: {::02x}", map.span());
    }
    println("Found: {} instances of string '{}'", newMaps.size(), strToFind);
}
