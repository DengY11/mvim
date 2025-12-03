#pragma once
#include <string>
#include <vector>
#include "i_text_buffer_core.hpp"

class VectorTextBufferCore : public ITextBufferCore {
public:
  void init_from_lines(const std::vector<std::string>& lines) override { lines_ = lines; }
  int line_count() const override { return (int)lines_.size(); }
  std::string get_line(int r) const override { if (r < 0 || r >= (int)lines_.size()) return std::string(); return lines_[r]; }

  void insert_line(int row, const std::string& s) override {
    if (row < 0) row = 0; if (row > (int)lines_.size()) row = (int)lines_.size();
    lines_.insert(lines_.begin() + row, s);
  }
  void insert_lines(int row, const std::vector<std::string>& ss) override {
    if (row < 0) row = 0; if (row > (int)lines_.size()) row = (int)lines_.size();
    lines_.insert(lines_.begin() + row, ss.begin(), ss.end());
  }
  void erase_line(int row) override {
    if (row < 0 || row >= (int)lines_.size()) return;
    lines_.erase(lines_.begin() + row);
    if (lines_.empty()) lines_.emplace_back("");
  }
  void erase_lines(int start_row, int end_row) override {
    if (start_row < 0) start_row = 0; if (end_row < start_row) end_row = start_row;
    if (start_row >= (int)lines_.size()) return; if (end_row > (int)lines_.size()) end_row = (int)lines_.size();
    lines_.erase(lines_.begin() + start_row, lines_.begin() + end_row);
    if (lines_.empty()) lines_.emplace_back("");
  }
  void replace_line(int row, const std::string& s) override {
    if (row < 0 || row >= (int)lines_.size()) return;
    lines_[row] = s;
  }

  const std::vector<std::string>& raw_lines() const { return lines_; }
private:
  std::vector<std::string> lines_;
};
