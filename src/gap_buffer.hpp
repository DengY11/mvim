#pragma once
#include <vector>
#include <string>

class GapBuffer {
public:
  std::vector<char> buf;
  size_t gap_start = 0;
  size_t gap_end = 0;

  void clear();
  size_t length() const;
  void init_from_lines(const std::vector<std::string>& lines);
  void move_gap_to(size_t pos);
  void ensure_gap(size_t need);
  void insert_text(const std::string& text);
  void erase_range(size_t pos, size_t len);
  std::string slice(size_t pos, size_t len) const;
};
