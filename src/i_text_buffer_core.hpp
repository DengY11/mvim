#pragma once
#include <string>
#include <vector>

class ITextBufferCore {
public:
  virtual ~ITextBufferCore() = default;
  virtual void init_from_lines(const std::vector<std::string>& lines) = 0;
  virtual int line_count() const = 0;
  virtual std::string get_line(int r) const = 0;

  virtual void insert_line(int row, const std::string& s) = 0;
  virtual void insert_lines(int row, const std::vector<std::string>& ss) = 0;
  virtual void erase_line(int row) = 0;
  virtual void erase_lines(int start_row, int end_row) = 0; // end_row exclusive
  virtual void replace_line(int row, const std::string& s) = 0;
};
