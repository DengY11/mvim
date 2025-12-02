#include "text_buffer_core.hpp"

void TextBufferCore::init_from_lines(const std::vector<std::string>& lines) {
  gb.clear();
  gb.init_from_lines(lines);
  li.build_from_text(gb.buf, gb.gap_start, gb.gap_end);
}

int TextBufferCore::line_count() const { return (int)li.line_count(); }

std::string TextBufferCore::get_line(int r) const {
  if (r < 0) return std::string();
  size_t start = li.line_start((size_t)r);
  size_t end;
  size_t total = gb.length();
  if ((size_t)r + 1 < li.line_count()) end = li.line_start((size_t)r + 1);
  else end = total;
  size_t len = (end > start) ? (end - start) : 0;
  if ((size_t)r + 1 < li.line_count()) {
    if (len > 0) len -= 1;
  }
  return gb.slice(start, len);
}
