#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <span>

/*
  currently we have rope, gap buffer backend and vector backend
  but gap buffer seems to be slower than vector backend
  rope is faster than gap buffer, but not widely tested
*/
class ITextBufferCore {
public:
  virtual ~ITextBufferCore() = default;
  virtual void init_from_lines(const std::vector<std::string>& lines) = 0;
  virtual int line_count() const = 0;
  virtual std::string get_line(int r) const = 0;
  virtual std::string_view get_name() const = 0;

  virtual void insert_line(size_t row, const std::string& s) = 0;
  virtual void insert_line(size_t row, std::string_view s) = 0;
  virtual void insert_lines(size_t row, const std::vector<std::string>& ss) = 0;
  virtual void insert_lines(size_t row, std::span<const std::string> ss) = 0;
  virtual void erase_line(size_t row) = 0;
  virtual void erase_lines(size_t start_row, size_t end_row) = 0; // end_row exclusive
  virtual void replace_line(size_t row, const std::string& s) = 0;
  virtual void replace_line(size_t row, std::string_view s) = 0;
};
