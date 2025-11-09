#include "mem_anal_instructions.hpp"
#include "logs.hpp"
#include "maps.hpp"
#include "mem_anal.hpp"

inline int
IR_ConvertIndex(int                                      original,
                const std::vector<RegionPropertiesList>& container)
{
    int actualIndex = original;
    if (actualIndex < 0)
        actualIndex += container.size();
    Log(Debug,
        "Converted index: " << original << " -> " << actualIndex);
    return actualIndex;
}

inline bool
IR_ReturnIfEmpty(const std::vector<RegionPropertiesList>& container)
{
    if (container.size() == 0)
    {
        Log(Error, "The Region Properties List Timeline is Empty!!!");
        return false;
    }
    return true;
}

inline bool
IR_checkArgCounts(const std::vector<std::string_view>& args,
                  size_t                               numArgs)
{
    if (args.size() != numArgs)
    {
        Log(Error,
            "Invalid argument amount of size "
                << args.size() << "! Requires " << numArgs << "!");
        return false;
    }
    return true;
}

const InstructionFunc IR_Invalid = [](ProgramAnalysisState&) {
    Log(Error, "Invalid instruction!");
};

// The explicit return types are required because deduction occurs before conversion to std::function.
// This means the stupid fucking compiler will think IR_Invalid is different from the outputted lambda.
const std::map<
    std::string,
    std::function<InstructionFunc(std::vector<std::string_view>)>>
    g_instructionRegistry = {
        //     {
        //         "template",
        //         [](const std::vector<std::string_view> &args)
        //         {
        //             return [](ProgramAnalysisState& s){};
        //         }
        //     },
        {instructionNames[0],           // ReadMaps
         [](const std::vector<std::string_view>&) -> InstructionFunc
         {
             return [](ProgramAnalysisState& s)
             { s.updateOriginalProperties(); };
         }            },
        {instructionNames[1],           // CopyOriginalMaps
         [](const std::vector<std::string_view>&) -> InstructionFunc
         {
             return [](ProgramAnalysisState& s)
             { s.copyOriginalPropertiesToTimeline(); };
         }            },
        {instructionNames[2],           // FilterRegionName
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         {
             if (!IR_checkArgCounts(args, 3))
                 return IR_Invalid;
             int64_t index = MA_stoi64(args[2]);
             return [=](ProgramAnalysisState& s)
             {
                 if (!IR_ReturnIfEmpty(s.m_rplTimeline))
                 {
                     return;
                 }
                 int64_t actualIndex =
                     IR_ConvertIndex(index, s.m_rplTimeline);
                 Log(Debug, "Filtering region by name: " << args[1]);
                 s.m_rplTimeline.push_back(
                     s.m_rplTimeline[actualIndex].filterRegionsByName(
                         args[1]));
             };
         }            },
        {instructionNames[3],           // FilterRegionSubName
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         {
             if (!IR_checkArgCounts(args, 3))
                 return IR_Invalid;
             int64_t index = MA_stoi64(args[2]);
             return [=](ProgramAnalysisState& s)
             {
                 if (!IR_ReturnIfEmpty(s.m_rplTimeline))
                     return;
                 int64_t actualIndex =
                     IR_ConvertIndex(index, s.m_rplTimeline);
                 s.m_rplTimeline.push_back(
                     s.m_rplTimeline[actualIndex]
                         .filterRegionsBySubName(args[1]));
             };
         }            },
        {instructionNames[4],           // FilterPerms
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         {
             if (!IR_checkArgCounts(args, 3))
                 return IR_Invalid;
             int64_t index = MA_stoi64(args[2]);
             return [=](ProgramAnalysisState& s)
             {
                 if (!IR_ReturnIfEmpty(s.m_rplTimeline))
                     return;
                 int64_t actualIndex =
                     IR_ConvertIndex(index, s.m_rplTimeline);
                 s.m_rplTimeline.push_back(
                     s.m_rplTimeline[actualIndex]
                         .filterRegionsByPerms(args[1]));
             };
         }            },
        {instructionNames[5],           // FilterHasPerms
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         {
             if (!IR_checkArgCounts(args, 3))
                 return IR_Invalid;
             int64_t index = MA_stoi64(args[2]);
             return [=](ProgramAnalysisState& s)
             {
                 if (!IR_ReturnIfEmpty(s.m_rplTimeline))
                     return;
                 int64_t actualIndex =
                     IR_ConvertIndex(index, s.m_rplTimeline);
                 s.m_rplTimeline.push_back(
                     s.m_rplTimeline[actualIndex]
                         .filterRegionsByHasPerms(args[1]));
             };
         }            },
        {instructionNames[6],           // FilterNotPerms
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         {
             if (!IR_checkArgCounts(args, 3))
                 return IR_Invalid;
             int64_t index = MA_stoi64(args[2]);
             return [=](ProgramAnalysisState& s)
             {
                 if (!IR_ReturnIfEmpty(s.m_rplTimeline))
                     return;
                 int64_t actualIndex =
                     IR_ConvertIndex(index, s.m_rplTimeline);
                 s.m_rplTimeline.push_back(
                     s.m_rplTimeline[actualIndex]
                         .filterRegionsByNotPerms(args[1]));
             };
         }            },
        {instructionNames[7],           // FilterActiveRegion
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         {
             if (!IR_checkArgCounts(args, 3))
                 return IR_Invalid;
             int64_t index = MA_stoi64(args[2]);
             return [=](ProgramAnalysisState& s)
             {
                 if (!IR_ReturnIfEmpty(s.m_rplTimeline))
                     return;
                 int64_t actualIndex =
                     IR_ConvertIndex(index, s.m_rplTimeline);
                 s.m_rplTimeline.push_back(
                     getActiveRegionsFromRegionPropertiesList(
                         s.m_rplTimeline[actualIndex]));
             };
         }            },
        {// "MakeSnapshot",
         instructionNames[8],
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         { return [](ProgramAnalysisState& s) {}; }},
        {// "FindChangingRegions",
         instructionNames[9],
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         { return [](ProgramAnalysisState& s) {}; }},
        {// "FindUnchangingRegions",
         instructionNames[10],
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         { return [](ProgramAnalysisState& s) {}; }},
        {// "FindString",
         instructionNames[11],
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         { return [](ProgramAnalysisState& s) {}; }},
        {// "FindNumeric",
         instructionNames[12],
         [](const std::vector<std::string_view>& args)
         { return [](ProgramAnalysisState& s) {}; }},
        {// "FindChangingNumeric",
         instructionNames[13],
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         { return [](ProgramAnalysisState& s) {}; }},
        {// "FindUnchangingNumeric",
         instructionNames[14],
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         { return [](ProgramAnalysisState& s) {}; }},
        {// "LoopStable",
         instructionNames[15],
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         { return [](ProgramAnalysisState& s) {}; }},
        {// "LoopRemain",
         instructionNames[16],
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         { return [](ProgramAnalysisState& s) {}; }},
        {// "Tag",
         instructionNames[17],
         [](const std::vector<std::string_view>& args)
             -> InstructionFunc
         { return [](ProgramAnalysisState& s) {}; }},
};
uint8_t asciiToBaseInf(char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';

    else if ('a' <= c && c <= 'z')
        return c - 87; // 'a' - 87 = 10

    else if ('A' <= c && c <= 'Z')
        return c - 55; // 'A' - 55 = 10

    // Invalid
    else
        return (uint8_t)~0;
}
