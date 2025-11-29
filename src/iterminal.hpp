#pragma once
/*
 * ITerminal
 *
 * 作用：终端/图形后端抽象；提供尺寸、清屏、绘制文本、光标移动、刷新等能力。
 * 目的：与具体实现（ncurses/headless/其他）解耦，支持多后端切换与测试桩。
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
