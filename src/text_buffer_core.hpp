#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "gap_buffer.hpp"
#include "line_index.hpp"

class TextBufferCore {
public:
  GapBuffer gb;
  LineIndex li;

  void init_from_lines(const std::vector<std::string>& lines);
  int line_count() const;
  std::string get_line(int r) const;
};
