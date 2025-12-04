#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <span>
#include <filesystem>
#include "i_text_buffer_core.hpp"
#include "gap_buffer.hpp"
#include "line_index.hpp"

/*
  this is the gap buffer backend
  btw, it's not steady version, may cause problem
  you can choose vector backend if you want a steady version
*/
class GapTextBufferCore : public TextBufferCoreCRTP<GapTextBufferCore> {
public:
  GapBuffer gb;
  LineIndex li;
  static constexpr std::string_view get_name_sv() { return "gap"; }

  void init_from_lines(const std::vector<std::string>& lines);
  int line_count() const;
  std::string get_line(int r) const;

  void insert_line(size_t row, const std::string& s);
  void insert_line(size_t row, std::string_view s);
  void insert_lines(size_t row, const std::vector<std::string>& ss);
  void insert_lines(size_t row, std::span<const std::string> ss);
  void erase_line(size_t row);
  void erase_lines(size_t start_row, size_t end_row); 
  void replace_line(size_t row, const std::string& s);
  void replace_line(size_t row, std::string_view s);
  /*forward to CRTP impl*/
  void do_init_from_lines(const std::vector<std::string>& lines) { init_from_lines(lines); }
  int do_line_count() const { return line_count(); }
  std::string do_get_line(int r) const { return get_line(r); }
  void do_insert_line(size_t row, const std::string& s) { insert_line(row, s); }
  void do_insert_line(size_t row, std::string_view s) { insert_line(row, s); }
  void do_insert_lines(size_t row, const std::vector<std::string>& ss) { insert_lines(row, ss); }
  void do_insert_lines(size_t row, std::span<const std::string> ss) { insert_lines(row, std::vector<std::string>(ss.begin(), ss.end())); }
  void do_erase_line(size_t row) { erase_line(row); }
  void do_erase_lines(size_t start_row, size_t end_row) { erase_lines(start_row, end_row); }
  void do_replace_line(size_t row, const std::string& s) { replace_line(row, s); }
  void do_replace_line(size_t row, std::string_view s) { replace_line(row, s); }
};
