#include "text_buffer.hpp"
#include "gap_text_buffer_core.hpp"
#include "vector_text_buffer_core.hpp"
#include "rope_text_buffer_core.hpp"
#include "rope_text_buffer_core.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <random>

struct BenchCfg {
  int N = 200000;               /*init lines count*/
  int get_iters = 200000;       /*get_line iters*/ 
  int insert_iters = 10000;     /*insert_line iters (head/mid/tail)*/
  int erase_iters = 10000;      /*erase_line iters (head/mid/tail)*/
  int block_size = 100;         /*batch insert/erase block size*/
  int block_iters = 500;        /*batch insert/erase iters (head/mid/tail)*/
};

static std::vector<std::string> make_lines(int n) {
  std::vector<std::string> lines;
  lines.reserve(n);
  for (int i = 0; i < n; ++i) lines.emplace_back("line");
  return lines;
}

template <typename Core>
static void bench_init_one(const char* tag, const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  Core core;
  auto t0 = std::chrono::steady_clock::now();
  core.init_from_lines(lines);
  auto t1 = std::chrono::steady_clock::now();
  std::chrono::duration<double> dt = t1 - t0;
  std::cout << tag << " init_from_lines N=" << cfg.N << " took " << dt.count() << "s\n";
}
static void bench_init(const BenchCfg& cfg) {
  bench_init_one<VectorTextBufferCore>("[vector]   ", cfg);
  bench_init_one<GapTextBufferCore>("[gap]      ", cfg);
  bench_init_one<RopeTextBufferCore>("[rope]     ", cfg);
}

static void bench_get_line(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, cfg.N - 1);
  auto bench_one = [&](const char* tag, auto& core){
    core.init_from_lines(lines);
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < cfg.get_iters; ++i) {
      volatile auto s = core.get_line(dist(rng));
      (void)s;
    }
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> dt = t1 - t0;
    std::cout << tag << " get_line iters=" << cfg.get_iters << " took " << dt.count() << "s\n";
  };
  {
    VectorTextBufferCore v; bench_one("[vector]   ", v);
    GapTextBufferCore g; bench_one("[gap]      ", g);
    RopeTextBufferCore r; bench_one("[rope]     ", r);
  }
}

static void bench_insert_line(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  auto run = [&](const char* tag, auto& core){
    core.init_from_lines(lines);
    // head
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.insert_iters; ++i) core.insert_line(0, std::string_view("x"));
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_line head iters=" << cfg.insert_iters << " took " << dt.count() << "s\n";
    }
    // mid
    {
      int mid = std::max(0, core.line_count()/2);
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.insert_iters; ++i) core.insert_line(mid, std::string_view("x"));
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_line mid  iters=" << cfg.insert_iters << " took " << dt.count() << "s\n";
    }
    // tail
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.insert_iters; ++i) core.insert_line(core.line_count(), std::string_view("x"));
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_line tail iters=" << cfg.insert_iters << " took " << dt.count() << "s\n";
    }
  };
  { VectorTextBufferCore v; run("[vector]  ", v); }
  { GapTextBufferCore g; run("[gap]     ", g); }
  { RopeTextBufferCore r; run("[rope]    ", r); }
}

static void bench_insert_lines(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  std::vector<std::string> block(cfg.block_size, std::string("blk"));
  auto run = [&](const char* tag, auto& core){
    core.init_from_lines(lines);
    // head
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) core.insert_lines(0, block);
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_lines head iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
    // mid
    {
      int mid = std::max(0, core.line_count()/2);
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) core.insert_lines(mid, block);
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_lines mid  iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
    // tail
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) core.insert_lines(core.line_count(), block);
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_lines tail iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
  };
  { VectorTextBufferCore v; run("[vector]  ", v); }
  { GapTextBufferCore g; run("[gap]     ", g); }
  { RopeTextBufferCore r; run("[rope]    ", r); }
}

static void bench_erase_line(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  auto run = [&](const char* tag, auto& core){
    core.init_from_lines(lines);
    // head
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.erase_iters; ++i) core.erase_line(0);
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_line  head iters=" << cfg.erase_iters << " took " << dt.count() << "s\n";
    }
    // mid
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.erase_iters; ++i) {
        int mid = std::max(0, core.line_count()/2 - 1);
        if (core.line_count() > 0) core.erase_line(mid);
      }
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_line  mid  iters=" << cfg.erase_iters << " took " << dt.count() << "s\n";
    }
    // tail
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.erase_iters; ++i) {
        if (core.line_count() > 0) core.erase_line(core.line_count()-1);
      }
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_line  tail iters=" << cfg.erase_iters << " took " << dt.count() << "s\n";
    }
  };
  { VectorTextBufferCore v; run("[vector]  ", v); }
  { GapTextBufferCore g; run("[gap]     ", g); }
  { RopeTextBufferCore r; run("[rope]    ", r); }
}

static void bench_erase_lines(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  auto run = [&](const char* tag, auto& core){
    core.init_from_lines(lines);
    // head
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) {
        int end = std::min(cfg.block_size, core.line_count());
        core.erase_lines(0, end);
      }
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_lines head iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
    // mid
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) {
        int mid = std::max(0, core.line_count()/2 - cfg.block_size/2);
        int end = std::min(core.line_count(), mid + cfg.block_size);
        if (end > mid) core.erase_lines(mid, end);
      }
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_lines mid  iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
    // tail
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) {
        int end = core.line_count();
        int start = std::max(0, end - cfg.block_size);
        if (end > start) core.erase_lines(start, end);
      }
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_lines tail iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
  };
  { VectorTextBufferCore v; run("[vector]  ", v); }
  { GapTextBufferCore g; run("[gap]     ", g); }
  { RopeTextBufferCore r; run("[rope]    ", r); }
}

static void bench_replace_line(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  std::mt19937 rng(54321);
  std::uniform_int_distribution<int> dist(0, cfg.N - 1);
  auto run = [&](const char* tag, auto& core){
    core.init_from_lines(lines);
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < cfg.get_iters; ++i) {
      int r = dist(rng);
      core.replace_line(static_cast<size_t>(r), std::string_view("rep"));
    }
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> dt = t1 - t0;
    std::cout << tag << " replace_line iters=" << cfg.get_iters << " took " << dt.count() << "s\n";
  };
  { VectorTextBufferCore v; run("[vector]  ", v); }
  { GapTextBufferCore g; run("[gap]     ", g); }
  { RopeTextBufferCore r; run("[rope]    ", r); }
}

int main(int argc, char** argv) {
  BenchCfg cfg;
  if (argc > 1) { try { cfg.N = std::stoi(argv[1]); } catch (...) {} }
  std::cout << "Backend operations benchmark (N=" << cfg.N << ")\n";
  bench_init(cfg);
  bench_get_line(cfg);
  bench_insert_line(cfg);
  bench_insert_lines(cfg);
  bench_erase_line(cfg);
  bench_erase_lines(cfg);
  bench_replace_line(cfg);
  return 0;
}
