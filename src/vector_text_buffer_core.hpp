#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <span>
#include "i_text_buffer_core.hpp"

class VectorTextBufferCore : public ITextBufferCore {
public:
  std::string_view get_name() const override { return "vector"; }

  void init_from_lines(const std::vector<std::string>& lines) override { lines_ = lines; }
  int line_count() const override { return static_cast<int>(lines_.size()); }
  std::string get_line(int r) const override { if (r < 0 || r >= static_cast<int>(lines_.size())) return std::string(); return lines_[r]; }

  void insert_line(size_t row, const std::string& s) override {
    size_t pos = std::min(row, lines_.size());
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(pos), s);
  }
  void insert_line(size_t row, std::string_view s) override {
    size_t pos = std::min(row, lines_.size());
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(pos), std::string(s));
  }
  void insert_lines(size_t row, const std::vector<std::string>& ss) override {
    size_t pos = std::min(row, lines_.size());
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(pos), ss.begin(), ss.end());
  }
  void insert_lines(size_t row, std::span<const std::string> ss) override {
    size_t pos = std::min(row, lines_.size());
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(pos), ss.begin(), ss.end());
  }
  void erase_line(size_t row) override {
    if (row >= lines_.size()) return;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(row));
    if (lines_.empty()) lines_.emplace_back("");
  }
  void erase_lines(size_t start_row, size_t end_row) override {
    if (end_row < start_row) end_row = start_row;
    start_row = std::min(start_row, lines_.size());
    end_row = std::min(end_row, lines_.size());
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(start_row),
                 lines_.begin() + static_cast<std::ptrdiff_t>(end_row));
    if (lines_.empty()) lines_.emplace_back("");
  }
  void replace_line(size_t row, const std::string& s) override {
    if (row >= lines_.size()) return;
    lines_[row] = s;
  }
  void replace_line(size_t row, std::string_view s) override {
    if (row >= lines_.size()) return;
    lines_[row] = std::string(s);
  }

  const std::vector<std::string>& raw_lines() const { return lines_; }
private:
  std::vector<std::string> lines_;
};
