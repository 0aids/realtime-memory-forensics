#include "mem_anal.hpp"
#include "core.hpp"
#include "gui.hpp"
#include "mem_anal_instructions.hpp"
#include "maps.hpp"
#include <ranges>
#include <tuple>
#include <functional>
#include <map>
#include <span>

template <Numeric NumType>
NumType evaluateArithmetic(const std::string_view &str)
{
    // guh why do i have to parse this.
}

// Only care about the highest precision.
template uint64_t evaluateArithmetic<uint64_t>(const std::string_view &str);
template double evaluateArithmetic<double>(const std::string_view &str);


// Very quick and dirty. Might not work as expected.
// Does check for speech mark enclosed text, and counts it as a single token.
// Does not support utf-8 or character escapes.
std::vector<std::string_view> parseLineIntoTokens(const char* cstr) {
    using namespace std::literals;
    const char* head = cstr;
    const char* tail = cstr;
    bool stacked = false;
    size_t off = 0;
    std::vector<std::string_view> strs;

    if (!cstr)
    {
        Log(Error, "Received an null cstr?");
        return {};
    }

    while (*head != '\0') {
        if (*head == '"') {
            stacked = !stacked;
            if (stacked) {
                tail++;
                off = 1;
            }
        }
        else if (*head == ' ' && !stacked) {
            if (head - tail - off > 0)
                strs.emplace_back(tail, head - tail - off);
            tail = head + 1;
            off = 0;
        }
        head++;
    }
    if (head - tail - off > 0)
        strs.emplace_back(tail, head - tail - off);
    std::string toLog = "{";
    for (auto &s : strs) {
        toLog += "(";
        toLog += s;
        toLog += "), ";
    }
    toLog += "}";
    Log(Debug, "Deduced tokens: " << toLog);

    return strs;
}

// Parse a line that is known to exist.
void ProgramAnalysisState::Instruction::parseLine() {
    const char* cstr = text.c_str();
    std::vector<std::string_view> tokens = parseLineIntoTokens(cstr) ;
    this->compiledText = g_instructionRegistry.at(std::string(tokens[0]))(tokens);
}
