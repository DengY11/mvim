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
#include "pane_layout.hpp"

struct PaneRenderInfo {
  const TextBuffer* buf = nullptr;
  Cursor cur{};
  Viewport* vp = nullptr;
  std::optional<std::filesystem::path> file_path;
  bool modified = false;
  bool is_active = false;
  Rect area{};
  int override_row = -1;
  std::string override_line;
};

class Renderer {
public:
  void render(ITerminal& term,
              const std::vector<PaneRenderInfo>& panes,
              Mode mode,
              const std::string& message,
              const std::string& cmdline,
              bool visual_active,
              Cursor visual_anchor,
              bool show_line_numbers,
              bool relative_line_numbers,
              bool enable_color,
              const std::vector<SearchHit>& search_hits);
};
