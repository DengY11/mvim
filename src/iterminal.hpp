#pragma once
/*
 * ITerminal
 *
 * Purpose: abstract terminal backend (size, clear, draw, cursor, refresh).
 * Goal: decouple from concrete impls (ncurses/headless/etc), enable testing.
 */
#include <string>

struct TermSize { int rows; int cols; };

class ITerminal {
public:
  virtual ~ITerminal() = default;
  virtual TermSize getSize() const = 0;
  virtual void clear() = 0;
  virtual void drawText(int row, int col, const std::string& text) = 0;
  virtual void drawHighlighted(int row, int col, const std::string& text, int hl_start, int hl_len) = 0;
  virtual void drawColored(int row, int col, const std::string& text, int color_pair_id) = 0;
  virtual void moveCursor(int row, int col) = 0;
  virtual void refresh() = 0;
};
