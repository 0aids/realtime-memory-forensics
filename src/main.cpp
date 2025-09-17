#include "logger.hpp"
#include "obtainMemory.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

extern "C" {
#include <ncurses.h>
#include <sched.h>
}

using namespace std;
int main(int argc, char *argv[]) {
  if (argc < 3) {
    ERROR("Not enough arguments!")
    ERROR("args: PID, String to search for")
    return 0;
  }
  cout << "Args: ";
  for (int i = 0; i < argc; i++) {
    cout << argv[i] << ", ";
  }
  cout << endl;

  pid_t pid = atoi(argv[1]);
  ProgramMemory pm(pid);
  string requestedString(argv[2]);
  pm.setup();
  pm.searchFirst(requestedString);

  // Initialize ncurses
  initscr();

  // Print a string to the screen
  printw("Hello, ncurses!");

  // Refresh the screen to show changes
  refresh();

  // Wait for user input
  getch();

  // De-initialize ncurses and restore terminal
  endwin();
}
