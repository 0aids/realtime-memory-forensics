#include "rmf/logging/logging.hpp"
#include "rmf/utils/expect.hpp"
#include <magic_enum/magic_enum.hpp>
namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;

#define fmtString       "From [{}:{} - {}] {}"
#define fmtStringSubseq fmtString "\n\t^->"
std::string mfu::Error::generateMsg(ErrorEnum e, const char* file,
                                    lineNumber_t line,
                                    const char*  function)
{
    return std::format(fmtString, file, line, function,
                       magic_enum::enum_name(e));
}
std::string mfu::Error::generateSubseqMsg(ErrorEnum    e,
                                          const char*  file,
                                          lineNumber_t line,
                                          const char*  function)
{
    return std::format(fmtStringSubseq, file, line, function,
                       magic_enum::enum_name(e));
}

mfu::Error::Error(ErrorEnum e, const char* file, lineNumber_t line,
                  const char* function) : m_what()
{
    m_what = generateMsg(e, file, line, function);
}

mfu::Error&& mfu::Error::update(ErrorEnum e, const char* file,
                                lineNumber_t line,
                                const char*  function)
{
    m_what = generateSubseqMsg(e, file, line, function) + m_what;
    depth++;
    return std::move(*this);
}

const char* mfu::Error::what() const noexcept
{
    return m_what.c_str();
}
