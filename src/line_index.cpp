#include "line_index.hpp"

static inline char at_logical(const std::vector<char>& buf, size_t gap_start, size_t gap_end, size_t i) {
  if (i < gap_start) return buf[i];
  return buf[i + (gap_end - gap_start)];
}

void LineIndex::build_from_text(const std::vector<char>& buf, size_t gap_start, size_t gap_end) {
  blocks.clear();
  size_t len = buf.size() - (gap_end - gap_start);
  std::vector<size_t> starts;
  starts.push_back(0);
  for (size_t i = 0; i < len; ++i) {
    char c = at_logical(buf, gap_start, gap_end, i);
    if (c == '\n') starts.push_back(i + 1);
  }
  size_t n = starts.size();
  for (size_t i = 0; i < n; i += block_size) {
    size_t end = std::min(n, i + block_size);
    LineBlock b;
    b.base_offset = starts[i];
    b.rel.reserve(end - i);
    for (size_t k = i; k < end; ++k) b.rel.push_back(starts[k] - b.base_offset);
    blocks.push_back(std::move(b));
  }
}

size_t LineIndex::line_count() const { if (blocks.empty()) return 0; size_t c = 0; for (const auto& b : blocks) c += b.rel.size(); return c; }

size_t LineIndex::line_start(size_t row) const {
  if (blocks.empty()) return 0;
  size_t acc = 0;
  for (const auto& b : blocks) {
    if (row < acc + b.rel.size()) {
      size_t idx = row - acc;
      return b.base_offset + b.rel[idx];
    }
    acc += b.rel.size();
  }
  return blocks.back().base_offset + blocks.back().rel.back();
}
