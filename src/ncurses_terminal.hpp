#pragma once
/*
 * NcursesTerminal
 *
 * Purpose: ITerminal implementation using ncurses for drawing and input mode.
 * Note: initialization/teardown is managed by Terminal RAII wrapper.
 */
#include "iterminal.hpp"
#include <ncurses.h>

class NcursesTerminal : public ITerminal {
public:
  NcursesTerminal();
  ~NcursesTerminal();
  TermSize getSize() const override;
  void clear() override;
  void drawText(int row, int col, const std::string& text) override;
  void drawHighlighted(int row, int col, const std::string& text, int hl_start, int hl_len) override;
  void drawColored(int row, int col, const std::string& text, int color_pair_id) override;
  void moveCursor(int row, int col) override;
  void refresh() override;
  void setBackground(short color);
private:
  short bg_color_ = -1; // -1: default background
};
