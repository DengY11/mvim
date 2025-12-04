#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <span>

template <typename Derived>
class TextBufferCoreCRTP {
public:
  std::string_view get_name() const { return as_const_derived().get_name_sv(); }
  void init_from_lines(const std::vector<std::string>& lines) { as_derived().do_init_from_lines(lines); }
  int line_count() const { return as_const_derived().do_line_count(); }
  std::string get_line(int r) const { return as_const_derived().do_get_line(r); }
  /*insert*/
  void insert_line(size_t row, const std::string& s) { as_derived().do_insert_line(row, s); }
  void insert_line(size_t row, std::string_view s) { as_derived().do_insert_line(row, s); }
  void insert_lines(size_t row, const std::vector<std::string>& ss) { as_derived().do_insert_lines(row, ss); }
  void insert_lines(size_t row, std::span<const std::string> ss) { as_derived().do_insert_lines(row, ss); }
  /*erase*/
  void erase_line(size_t row) { as_derived().do_erase_line(row); }
  void erase_lines(size_t start_row, size_t end_row) { as_derived().do_erase_lines(start_row, end_row); }
  /*replace*/
  void replace_line(size_t row, const std::string& s) { as_derived().do_replace_line(row, s); }
  void replace_line(size_t row, std::string_view s) { as_derived().do_replace_line(row, s); }

private:
  Derived& as_derived() { return static_cast<Derived&>(*this); }
  const Derived& as_const_derived() const { return static_cast<const Derived&>(*this); }
};
