#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "i_text_buffer_core.hpp"
#include "gap_buffer.hpp"
#include "line_index.hpp"

/*
  this is the gap buffer backend
  btw, it's not steady version, may cause problem
  you can choose vector backend if you want a steady version
*/
class GapTextBufferCore : public ITextBufferCore {
public:
  GapBuffer gb;
  LineIndex li;

  void init_from_lines(const std::vector<std::string>& lines) override;
  int line_count() const override;
  std::string get_line(int r) const override;

  void insert_line(int row, const std::string& s) override;
  void insert_lines(int row, const std::vector<std::string>& ss) override;
  void erase_line(int row) override;
  void erase_lines(int start_row, int end_row) override; // end_row exclusive
  void replace_line(int row, const std::string& s) override;
};
