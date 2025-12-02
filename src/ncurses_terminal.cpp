#include "ncurses_terminal.hpp"
#include <cwchar>
#include <vector>

NcursesTerminal::NcursesTerminal() {
  if (has_colors()) {
    start_color();
    if (use_default_colors() == OK) {
      init_pair(1, COLOR_YELLOW, -1); // keyword: default background
    } else {
      init_pair(1, COLOR_YELLOW, COLOR_BLACK); // fallback
    }
    // default background pair
    init_pair(2, -1, -1);
  }
}
NcursesTerminal::~NcursesTerminal() {}

TermSize NcursesTerminal::getSize() const {
  int r, c; getmaxyx(stdscr, r, c); return {r, c};
}

void NcursesTerminal::clear() { erase(); }

void NcursesTerminal::drawText(int row, int col, const std::string& text) {
  if (has_colors()) attron(COLOR_PAIR(2));
  mvaddnstr(row, col, text.c_str(), (int)text.size());
  if (has_colors()) attroff(COLOR_PAIR(2));
}

void NcursesTerminal::drawHighlighted(int row, int col, const std::string& text, int hl_start, int hl_len) {
  int cols = getSize().cols;
  int len = (int)text.size();
  if (hl_start < 0) hl_start = 0;
  if (hl_len < 0) hl_len = 0;
  int hl_end = std::min(len, hl_start + hl_len);
  hl_start = std::min(hl_start, len);
  if (len == 0) {
    int rem = std::max(0, cols - col);
    if (rem > 0) {
      std::string spaces(rem, ' ');
      attron(A_REVERSE);
      mvaddnstr(row, col, spaces.c_str(), rem);
      attroff(A_REVERSE);
    }
    return;
  }
  if (hl_start > 0) {
    std::string left = text.substr(0, hl_start);
    mvaddnstr(row, col, left.c_str(), (int)left.size());
    col += (int)left.size();
  }
  if (hl_end > hl_start) {
    std::string mid = text.substr(hl_start, hl_end - hl_start);
    attron(A_REVERSE);
    mvaddnstr(row, col, mid.c_str(), (int)mid.size());
    attroff(A_REVERSE);
    col += (int)mid.size();
  }
  if (hl_end < len) {
    std::string right = text.substr(hl_end);
    mvaddnstr(row, col, right.c_str(), (int)right.size());
  }
}

void NcursesTerminal::drawColored(int row, int col, const std::string& text, int color_pair_id) {
  if (has_colors()) attron(COLOR_PAIR(color_pair_id));
  mvaddnstr(row, col, text.c_str(), (int)text.size());
  if (has_colors()) attroff(COLOR_PAIR(color_pair_id));
}

void NcursesTerminal::moveCursor(int row, int col) { move(row, col); }

void NcursesTerminal::refresh() { ::refresh(); }

void NcursesTerminal::setBackground(short color) {
  if (!has_colors()) return;
  bg_color_ = color;
  // Window background uses pair 2
  init_pair(2, -1, bg_color_);
  wbkgd(stdscr, COLOR_PAIR(2));
  // Keep keyword pair background in sync
  init_pair(1, COLOR_YELLOW, bg_color_);
  erase();
}
void NcursesTerminal::clearToEOL(int row, int col) {
  move(row, col);
  clrtoeol();
}
