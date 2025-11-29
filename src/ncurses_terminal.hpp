#pragma once
/*
 * NcursesTerminal
 *
 * 作用：ITerminal 的 ncurses 实现，绑定终端缓冲与输入模式。
 * 说明：仅提供绘制相关接口实现；初始化/还原由 Terminal RAII 管理。
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
