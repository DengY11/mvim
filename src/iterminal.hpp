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
  virtual void draw_text(int row, int col, const std::string& text) = 0;
  virtual void draw_highlighted(int row, int col, const std::string& text, int hl_start, int hl_len) = 0;
  virtual void draw_colored(int row, int col, const std::string& text, int color_pair_id) = 0;
  virtual void move_cursor(int row, int col) = 0;
  virtual void refresh() = 0;
  virtual void clear_to_eol(int row, int col) = 0;
};
