#include "terminal.hpp"
#include <locale.h>

Terminal::Terminal() {
  setlocale(LC_ALL, "");
  initscr();
  raw();
  noecho();
  keypad(stdscr, TRUE);
  ESCDELAY = 25;
}

Terminal::~Terminal() {
  endwin();
}
