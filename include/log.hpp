// Just a basic logging system.

#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3

#define ERROR_COLOR "\x1b[1;31m"
#define WARN_COLOR "\x1b[33m"
#define INFO_COLOR "\x1b[39m"
#define DEBUG_COLOR "\x1b[2;39m"
#define END_COLOR "\x1b[0m"

#ifndef LOG_LEVEL

#define LOG_LEVEL LOG_LEVEL_DEBUG

#endif

#define p_log_message(log_color, level_name, contents)                         \
  std::cerr << log_color << "[" level_name "] " __FILE__ ":" << __func__       \
            << ":" << __LINE__ << " -- " << contents << END_COLOR "\n"

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define p_log_debug(format) p_log_message(DEBUG_COLOR, " DEBUG ", format)

#define p_log_var(var_name)                                                    \
  p_log_message(DEBUG_COLOR, " DEBUG ", #var_name << " = " << var_name)

#define p_log_var_hex(var_name)                                                \
  p_log_message(DEBUG_COLOR, " DEBUG ",                                        \
                #var_name << " = " << std::showbase << std::hex << var_name    \
                          << std::dec);

#else
#define LOG_DEBUG(format, ...)                                                 \
  do {                                                                         \
  } while (0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define p_log_info(format) p_log_message(INFO_COLOR, " INFO  ", format)
#else
#define LOG_INFO(format, ...)                                                  \
  do {                                                                         \
  } while (0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARNING
#define p_log_warning(format) p_log_message(WARN_COLOR, "WARNING", format)
#else
#define LOG_WARNING(format, ...)                                               \
  do {                                                                         \
  } while (0)
#endif

#define p_log_error(format) p_log_message(ERROR_COLOR, "!ERROR!", format)
