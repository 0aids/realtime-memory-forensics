#ifndef mem_anal_instructions_hpp_INCLUDED
#define mem_anal_instructions_hpp_INCLUDED
#include "maps.hpp"
#include "mem_anal.hpp"
#include <map>
#include <string>

const char* const instructionNames[] = {
    "ReadMaps",
    "CopyOriginalMaps",
    "FilterRegionName",
    "FilterRegionSubName",
    "FilterPerms",
    "FilterHasPerms",
    "FilterNotPerms",
    "FilterActiveRegion",
    "MakeSnapshot",
    "FindChangingRegions",
    "FindUnchangingRegions",
    "FindString",
    "FindNumeric",
    "FindChangingNumeric",
    "FindUnchangingNumeric",
    "LoopStable",
    "LoopRemain",
    "Tag",
    "Pause", 
    "Chunkify",
    "QueuedThreadPool",
    "Sleepms",
};

uint8_t asciiToBaseInf(char c); 


// Errors are set to be INT64_MIN
template <typename T>
requires std::ranges::contiguous_range<T>
int64_t MA_stoi64(const T &str) 
{
    if (str.size() == 0) {
        // Log(Error, "Invalid length string passed to MA_stoi64");
        return INT64_MIN;
    }
    auto head = str.begin();
    int64_t value = 0;

    while (head != str.end() && !strchr("-+1234567890", *head)){
        if (*head != ' ') {
            value = INT64_MIN;
            return value;
        }
        head++;
    }
    int64_t sign = 1;
    int64_t base = 10;
    if (*head == '+' ){
        head++;
    }
    else if ('-' == *head) {
        sign = -1;
        head++;
    } else if (
        *head == '0' &&
        (head + 1) != str.end() && 
        strchr("xX", *(head + 1)))
    {
        base = 16;
        head+=2;
    }
    else if (
        *head == '0' &&
        (head + 1) != str.end() && 
        strchr("Oo", *(head + 1)))
    {
        base = 8;
        head+=2;
    }

    // Get rid of leading spaces.
    while (head != str.end()) {
        while (head != str.end() && !strchr("1234567890abcedfABCDEF", *head))
        {
            if (*head != ' ') goto MA_stoi64_end;
            head++;
        }

        if (head == str.end()) break;

        uint8_t toAdd = asciiToBaseInf(*head);
        // intentional error
        if (toAdd == (uint8_t)~0 || toAdd >= base) break;

        // Ignore overflows.
        value *= base;
        value += toAdd;

        head++;
    }

MA_stoi64_end: 

    return sign * value;
}
using InstructionFunc = std::function<void(ProgramAnalysisState&)>;

extern const std::map<std::string, std::function<InstructionFunc(std::vector<std::string_view>)>> 
    g_instructionRegistry; 

#endif // mem_anal_instructions_hpp_INCLUDED
