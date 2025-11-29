#pragma once
/*
 * Renderer
 *
 * 作用：负责文本区/状态栏/命令行渲染与视口滚动；不直接依赖 ncurses。
 * 依赖：通过 ITerminal 抽象绘制与光标移动，便于替换渲染后端。
 * 约束：保持无业务状态，接受 Editor 状态的只读快照进行绘制。
 */
#include <string>
#include <optional>
#include <filesystem>
#include "text_buffer.hpp"
#include "types.hpp"
#include "iterminal.hpp"

class Renderer {
public:
  void render(ITerminal& term,
              const TextBuffer& buf,
              const Cursor& cur,
              Viewport& vp,
              Mode mode,
              const std::optional<std::filesystem::path>& file_path,
              bool modified,
              const std::string& message,
              const std::string& cmdline,
              bool visual_active,
              Cursor visual_anchor,
              bool show_line_numbers,
              bool enable_color);
};
