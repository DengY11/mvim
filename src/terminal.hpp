#pragma once
/*
 * Terminal
 *
 * Purpose: RAII wrapper around ncurses init/teardown.
 * Usage: construct in main; destructor restores terminal.
 * Note: manages terminal modes (raw/noecho/keypad), not rendering.
 */
#include <ncurses.h>

class Terminal {
public:
  Terminal();
  ~Terminal();
};
