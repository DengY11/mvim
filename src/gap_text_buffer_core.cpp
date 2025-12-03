#include "gap_text_buffer_core.hpp"

void GapTextBufferCore::init_from_lines(const std::vector<std::string>& lines) {
  gb.clear();
  gb.init_from_lines(lines);
  li.build_from_text(gb.buf, gb.gap_start, gb.gap_end);
}

int GapTextBufferCore::line_count() const { return (int)li.line_count(); }

std::string GapTextBufferCore::get_line(int r) const {
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

static inline void rebuild(GapTextBufferCore& c) {
  c.li.build_from_text(c.gb.buf, c.gb.gap_start, c.gb.gap_end);
}

void GapTextBufferCore::insert_line(int row, const std::string& s) {
  int L = line_count();
  if (row < 0) row = 0; if (row > L) row = L;
  if (L == 0) {
    gb.move_gap_to(0);
    gb.insert_text(s);
    rebuild(*this);
    return;
  }
  if (row < L) {
    size_t pos = li.line_start((size_t)row);
    gb.move_gap_to(pos);
    std::string ins = s; ins.push_back('\n');
    gb.insert_text(ins);
  } else {
    size_t pos = gb.length();
    gb.move_gap_to(pos);
    std::string ins; ins.push_back('\n'); ins += s;
    gb.insert_text(ins);
  }
  rebuild(*this);
}

void GapTextBufferCore::insert_lines(int row, const std::vector<std::string>& ss) {
  if (ss.empty()) return;
  int L = line_count();
  if (row < 0) row = 0; if (row > L) row = L;
  std::string joined;
  for (size_t i = 0; i < ss.size(); ++i) {
    joined += ss[i]; if (i + 1 < ss.size()) joined.push_back('\n');
  }
  if (L == 0) {
    gb.move_gap_to(0);
    gb.insert_text(joined);
    rebuild(*this);
    return;
  }
  if (row < L) {
    size_t pos = li.line_start((size_t)row);
    gb.move_gap_to(pos);
    joined.push_back('\n');
    gb.insert_text(joined);
  } else {
    size_t pos = gb.length();
    gb.move_gap_to(pos);
    std::string ins; ins.push_back('\n'); ins += joined;
    gb.insert_text(ins);
  }
  rebuild(*this);
}

void GapTextBufferCore::erase_line(int row) {
  int L = line_count();
  if (row < 0 || row >= L) return;
  size_t start = li.line_start((size_t)row);
  size_t end = (row + 1 < L) ? li.line_start((size_t)row + 1) : gb.length();
  gb.erase_range(start, end - start);
  rebuild(*this);
}

void GapTextBufferCore::erase_lines(int start_row, int end_row) {
  int L = line_count();
  if (start_row < 0) start_row = 0;
  if (end_row < start_row) end_row = start_row;
  if (start_row >= L) return;
  if (end_row > L) end_row = L;
  size_t start = li.line_start((size_t)start_row);
  size_t end = (end_row < L) ? li.line_start((size_t)end_row) : gb.length();
  gb.erase_range(start, end - start);
  rebuild(*this);
}

void GapTextBufferCore::replace_line(int row, const std::string& s) {
  int L = line_count();
  if (row < 0 || row >= L) return;
  size_t start = li.line_start((size_t)row);
  size_t end = (row + 1 < L) ? li.line_start((size_t)row + 1) : gb.length();
  size_t len = end - start;
  size_t content_len = (row + 1 < L && len > 0) ? (len - 1) : len;
  gb.move_gap_to(start);
  gb.erase_range(start, content_len);
  gb.insert_text(s);
  rebuild(*this);
}
