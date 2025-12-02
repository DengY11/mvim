#pragma once
#include <vector>
#include <cstddef>

struct LineBlock {
  size_t base_offset;
  std::vector<size_t> rel;
};

class LineIndex {
public:
  std::vector<LineBlock> blocks;
  size_t block_size = 1024;

  void build_from_text(const std::vector<char>& buf, size_t gap_start, size_t gap_end);
  size_t line_count() const;
  size_t line_start(size_t row) const;
};
