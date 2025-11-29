#include "terminal.hpp"

Terminal::Terminal() {
  initscr();
  raw();
  noecho();
  keypad(stdscr, TRUE);
  ESCDELAY = 25;
}

Terminal::~Terminal() {
  endwin();
}

