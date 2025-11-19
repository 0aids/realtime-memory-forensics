#include "backends/mt_backend.hpp"
#include "scripting_engine.hpp"
#include "utils/logs.hpp"
#include "data/maps.hpp"
#include "backends/core.hpp"
#include "backends/ibackend.hpp"
#include "tests.hpp"
#include "data/snapshots.hpp"
#include <chrono>
#include <thread>
#include <filesystem>

int main()
{
    using namespace std;
    rmf::scripting::Scanner scanner("tests/scripting_engine_test_file.rmf");
    scanner.parseFile();

    rmf_Log(rmf_Message, "Complete!");
}
