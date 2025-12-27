#include "scripting_engine.hpp"
#include "utils/logs.hpp"
#include <chrono>
#include <magic_enum/magic_enum.hpp>
#include <cstring>
#include <exception>
#include <fstream>
#include <stdexcept>

using namespace rmf::scripting;
Scanner::Scanner(const std::filesystem::path& filename)
{
    std::ifstream stream(filename);

    if (!stream.is_open())
    {
        rmf_Log(
            rmf_Error,
            "Failed to open file: " << std::quoted(filename.c_str()));
        rmf_Log(rmf_Error, "Reason: " << std::strerror(errno));
        throw std::exception();
    }

    m_fileContents << stream.rdbuf();

    if (stream.fail())
    {
        rmf_Log(
            rmf_Error,
            "Failed to read file: " << std::quoted(filename.c_str()));
        rmf_Log(rmf_Error, "Reason: " << std::strerror(errno));
        throw std::exception();
    }

    return;
}

void Scanner::skipSpaces()
{
    while (!m_fileContents.eof() && m_fileContents.peek() == ' ')
        m_fileContents.get();
}
enum class e_DurationType : uint8_t
{
    null,
    s,  // normal seconds
    ms, // milli
    us, //micro
    ns  // nano
};

e_DurationType getDurationType(std::istream& is)
{
    auto pos = is.tellg();
    if (pos == -1)
        return e_DurationType::null;
    char buf[2];

    is.read(buf, 2);
    std::streamsize readCount = is.gcount();
    is.clear();
    is.seekg(pos);
    if (readCount == 0)
        return e_DurationType::null;

    std::string_view suffix(buf, readCount);
    if (suffix == "s")
    {
        is.seekg(pos + 1l);
        return e_DurationType::s;
    }
    if (suffix == "ms")
    {
        is.seekg(pos + 2l);
        return e_DurationType::ms;
    }
    if (suffix == "us")
    {
        is.seekg(pos + 2l);
        return e_DurationType::us;
    }
    if (suffix == "ns")
    {
        is.seekg(pos + 2l);
        return e_DurationType::ns;
    }
    return e_DurationType::null;
}

Token Scanner::parseNumber(char curChar)
{
    using namespace std::string_literals;
    // Parses doubles, hex, octal, decimal.
    // curChar can be '0-9', '.', '+', '-'.
    std::stringstream ss;
    ss << curChar;
    e_TokenType curType = e_TokenType::NumLit;
    while (!m_fileContents.eof())
    {
        curChar = m_fileContents.get();
        if (!"0123456789_.xoXOeE+-"s.contains(curChar))
        {
            m_fileContents.unget();
            break;
        }
        if (curChar == '.')
        {
            curType = e_TokenType::FltLit;
        }
        ss << curChar;
    }
    const e_DurationType dtype = getDurationType(m_fileContents);

    if (dtype != e_DurationType::null)
    {
        try
        {
            auto                     numValue = std::stoll(ss.str());
            std::chrono::nanoseconds duration;
            switch (dtype)
            {
                case e_DurationType::s:
                    duration = std::chrono::seconds(numValue);
                    break;
                case e_DurationType::ms:
                    duration = std::chrono::milliseconds(numValue);
                    break;
                case e_DurationType::us:
                    duration = std::chrono::microseconds(numValue);
                    break;
                case e_DurationType::ns:
                    duration = std::chrono::nanoseconds(numValue);
                    break;
            }
            return {e_TokenType::DurationLit, duration};
        }
        catch (std::invalid_argument& arg)
        {
            rmf_Log(rmf_Error,
                    "Error parsing duration - Not valid number: "
                        << ss.str());
            throw e_ErrorTypes::LexicalError;
        }
    }

    if (curType == e_TokenType::FltLit)
    {
        try
        {
            return {curType, std::stod(ss.str())};
        }
        catch (std::invalid_argument& arg)
        {
            rmf_Log(rmf_Error,
                    "Error parsing number - Not valid double: "
                        << ss.str());
            throw e_ErrorTypes::LexicalError;
        }
    }
    try
    {
        return {curType, std::stoll(ss.str())};
    }
    catch (std::invalid_argument& arg)
    {
        rmf_Log(rmf_Error,
                "Error parsing digit - Not valid integer"
                    << ss.str());
        throw e_ErrorTypes::LexicalError;
    }
}

Token Scanner::parseSymbolOrKeyword(char curChar)
{
    using namespace std::string_literals;
    std::stringstream ss;
    ss << curChar;
    while (!m_fileContents.eof() &&
           !" -+=!@#$%^&*(){};:/.,'\"|<>\\\n"s.contains(curChar = m_fileContents.peek()))
    {
        m_fileContents.get();
        ss << curChar;
    }
    if (ss.str() == "if")
    {
        return {e_TokenType::If, {}};
    }
    else if (ss.str() == "end")
    {
        return {e_TokenType::End, {}};
    }
    else if (ss.str() == "delete")
    {
        return {e_TokenType::Delete, {}};
    }
    else if (ss.str() == "for")
    {
        return {e_TokenType::For, {}};
    }
    else if (ss.str() == "in")
    {
        return {e_TokenType::In, {}};
    }
    else if (ss.str() == "sleep")
    {
        return {e_TokenType::Sleep, {}};
    }

    return {e_TokenType::Symbol, ss.str()};
}

Token Scanner::parseStringLiteral(char curChar)
{
    // Check if it's a multi-line string or not.
    char buf[3] = {0};
    auto pos = m_fileContents.tellg();
    std::string nullstr;
    std::string stringContents;

    m_fileContents.read(buf, 2);
    std::streamsize readCount = m_fileContents.gcount();
    m_fileContents.clear();
    m_fileContents.seekg(pos);
    // Determine indentation? Screw indentation im just going to yolo
    if (readCount == 2 && buf[0] == buf[1] && buf[0] == '"') {
        // Is a multi-line string.
        memset(buf, 0, 2);
        std::getline(m_fileContents, nullstr);
        while (!m_fileContents.eof()) {
            buf[2] = buf[1];
            buf[1] = buf[0];
            buf[0] = m_fileContents.get();
            if (!memcmp(buf, "\"\"\"", 3)) {
                // Pop back the extra '"'
                stringContents.pop_back();
                stringContents.pop_back();
                return {e_TokenType::StrLit, stringContents};
            }
            stringContents.push_back(buf[0]);
        }
        rmf_Log(rmf_Error, "No closing \"\"\" for multi-line string!");
        throw e_ErrorTypes::LexicalError;
    }
    while (!m_fileContents.eof()) {
        curChar = m_fileContents.get();
        if (curChar == '"') {
            return {e_TokenType::StrLit, stringContents};
        }
        stringContents.push_back(curChar);
    }
    rmf_Log(rmf_Error, "No closing \" for string!");
    throw e_ErrorTypes::LexicalError;

}

Token Scanner::parseNextToken()
{
    using namespace std::string_literals;
    // Some basic state for keeping track of our current token
    // Spaces are not significant. Scope is controlled by
    // end keywords.
    // Gotos are used here as a psuedo-while-loop because i'm lazy.
ParseBegin:
    skipSpaces();

    if (m_fileContents.eof())
    {
        return {e_TokenType::Eof, {}};
    }
    char        curChar = m_fileContents.get();
    std::string nullstr;

    switch (curChar)
    {
        case '#':
        {
            std::getline(m_fileContents, nullstr);
            goto ParseBegin;
            break;
        }
        case '\n':
        {
            goto ParseBegin;
            break;
        }
        // Check for negative/positive numeration
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': return parseNumber(curChar);
        case '+':
            if (!m_fileContents.eof() &&
                "0123456789"s.contains(m_fileContents.peek()))
            {
                return parseNumber(curChar);
            }
            return {e_TokenType::Plus, {}};

        case '-':
            if (!m_fileContents.eof() &&
                "0123456789"s.contains(m_fileContents.peek()))
            {
                return parseNumber(curChar);
            }
            return {e_TokenType::Minus, {}};

        case '/': return {e_TokenType::Divide, {}};
        case '*': return {e_TokenType::Times, {}};
        case '=': return {e_TokenType::Assignment, {}};
        case '(': return {e_TokenType::OpenParen, {}};
        case ')': return {e_TokenType::CloseParen, {}};
        case '[': return {e_TokenType::OpenBracket, {}};
        case ']': return {e_TokenType::CloseBracket, {}};
        case '{': return {e_TokenType::OpenBrace, {}};
        case '}': return {e_TokenType::CloseBrace, {}};
        case '.': return {e_TokenType::Dot, {}};
        case ',': return {e_TokenType::Comma, {}};
        case ';': return {e_TokenType::SemiColon, {}};
        case ':': return {e_TokenType::Colon, {}};
        case '"': return parseStringLiteral(curChar);
        default: break;
    }
    try
    {
        return parseSymbolOrKeyword(curChar);
    }
    catch (e_ErrorTypes err)
    {
        // Very terrible errors for now.
        rmf_Log(rmf_Error, "Failed to tokenise! Lexical error!");
        throw e_ErrorTypes::LexicalError;
    }
}

void Scanner::parseFile()
{
    while (!m_fileContents.eof())
    {
        try
        {
            m_tokens.push_back(parseNextToken());
            rmf_Log(rmf_Debug, "Found token: " << magic_enum::enum_name(m_tokens.back().type));
        }
        catch (e_ErrorTypes& error)
        {
            rmf_Log(
                rmf_Error,
                "Encountered an error while tokenising the script!");
            std::terminate();
        }
    }
}
