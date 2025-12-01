#pragma once
/*
 * Renderer
 *
 * Purpose: render text/status/command line and manage viewport scrolling.
 * Dependency: draws via ITerminal to allow backend replacement.
 * Constraint: stateless; receives snapshots from Editor to render.
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
