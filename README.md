# Memory analysis

A tool for recognising patterns within a memory for fast identification of
perhaps hidden information that is required.

# Notes before running

- Ensure that yama is set to 0 via
  ```bash
  echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
  ```
  And reset it afterwards by echoing `1` instead.
