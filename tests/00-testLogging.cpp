#include "log.hpp"

int main() {
  p_log_debug("This is a debug test");
  p_log_info("This is an info test");
  p_log_warning("This is a warning test");
  p_log_error("This is an error test");
  return 0;
}
