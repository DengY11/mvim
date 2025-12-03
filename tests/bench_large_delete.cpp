#include "text_buffer.hpp"
#include "gap_buffer.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <iostream>

int main(int argc, char** argv) {
  int N = 10000000; 
  if (argc > 1) {
    try { N = std::stoi(argv[1]); } catch (...) {}
  }
  int del_count = std::min(1000000, N / 2); 
  int del_start = N / 2 - del_count / 2;
  int del_end = del_start + del_count;
  TextBuffer b;
  {
    std::vector<std::string> lines;
    lines.reserve(N);
    for (int i = 0; i < N; ++i) lines.emplace_back("line");
    b.init_from_lines(std::move(lines));
  }
  auto t0 = std::chrono::steady_clock::now();
  b.erase_lines(del_start, del_end);
  auto t1 = std::chrono::steady_clock::now();
  std::chrono::duration<double> dt_vec = t1 - t0;
  std::cout << "[vector<string>] erase range took " << dt_vec.count()
            << "s, lines=" << b.line_count() << "\n";
  GapBuffer gb;
  {
    std::vector<std::string> lines;
    lines.reserve(N);
    for (int i = 0; i < N; ++i) lines.emplace_back("line");
    gb.init_from_lines(lines);
  }
  size_t pos = (size_t)del_start * 5;
  size_t len = (size_t)del_count * 5;
  auto g0 = std::chrono::steady_clock::now();
  gb.erase_range(pos, len);
  auto g1 = std::chrono::steady_clock::now();
  std::chrono::duration<double> dt_gap = g1 - g0;
  std::cout << "[gap_buffer]   erase range took " << dt_gap.count()
            << "s, lines_expected=" << (N - del_count)
            << ", text_len=" << gb.length() << "\n";

  return 0;
}
