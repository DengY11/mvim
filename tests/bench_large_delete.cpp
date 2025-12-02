#include "text_buffer.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <iostream>

int main() {
  const int N = 10000000;
  TextBuffer b;
  b.lines.reserve(N);
  for (int i = 0; i < N; ++i) b.lines.emplace_back("line");
  auto t0 = std::chrono::steady_clock::now();
  b.erase_lines(N/2 - 500000, N/2 + 500000);
  auto t1 = std::chrono::steady_clock::now();
  std::chrono::duration<double> dt = t1 - t0;
  std::cout << "erase range took " << dt.count() << "s, lines=" << b.line_count() << "\n";
  return 0;
}
