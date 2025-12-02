#include "text_buffer.hpp"
#include <cassert>
#include <string>
#include <vector>

int main() {
  TextBuffer b;
  b.lines = {"a", "b", "c"};
  assert(b.line_count() == 3);
  b.insert_line(1, "x");
  assert(b.line_count() == 4);
  assert(b.line(1) == std::string("x"));
  b.erase_line(2);
  assert(b.line_count() == 3);
  assert(b.line(0) == std::string("a"));
  assert(b.line(1) == std::string("x"));
  assert(b.line(2) == std::string("c"));
  b.replace_line(2, "z");
  assert(b.line(2) == std::string("z"));
  b.erase_lines(1, 3);
  assert(b.line_count() == 1);
  assert(b.line(0) == std::string("a"));
  return 0;
}
