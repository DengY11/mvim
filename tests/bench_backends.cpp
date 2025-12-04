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
  int N = 200000;               // 初始行数
  int get_iters = 200000;       // get_line 次数
  int insert_iters = 10000;     // insert_line 次数（头/中/尾各）
  int erase_iters = 10000;      // erase_line 次数（头/中/尾各）
  int block_size = 100;         // 批量插入/删除的块大小
  int block_iters = 500;        // 批量插入/删除迭代次数（头/中/尾各）
};

static std::vector<std::string> make_lines(int n) {
  std::vector<std::string> lines;
  lines.reserve(n);
  for (int i = 0; i < n; ++i) lines.emplace_back("line");
  return lines;
}

static void bench_init(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  // Vector
  {
    TextBuffer b; b.core = std::make_unique<VectorTextBufferCore>();
    auto t0 = std::chrono::steady_clock::now();
    b.init_from_lines(lines);
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> dt = t1 - t0;
    std::cout << "[vector]   init_from_lines N=" << cfg.N << " took " << dt.count() << "s\n";
  }
  // Gap
  {
    TextBuffer b; b.core = std::make_unique<GapTextBufferCore>();
    auto t0 = std::chrono::steady_clock::now();
    b.init_from_lines(lines);
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> dt = t1 - t0;
    std::cout << "[gap]      init_from_lines N=" << cfg.N << " took " << dt.count() << "s\n";
  }
}

static void bench_get_line(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, cfg.N - 1);
  // Vector
  {
    TextBuffer b; b.core = std::make_unique<VectorTextBufferCore>();
    b.init_from_lines(lines);
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < cfg.get_iters; ++i) {
      volatile auto s = b.line(dist(rng));
      (void)s;
    }
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> dt = t1 - t0;
    std::cout << "[vector]   get_line iters=" << cfg.get_iters << " took " << dt.count() << "s\n";
  }
  // Gap
  {
    TextBuffer b; b.core = std::make_unique<GapTextBufferCore>();
    b.init_from_lines(lines);
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < cfg.get_iters; ++i) {
      volatile auto s = b.line(dist(rng));
      (void)s;
    }
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> dt = t1 - t0;
    std::cout << "[gap]      get_line iters=" << cfg.get_iters << " took " << dt.count() << "s\n";
  }
}

static void bench_insert_line(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  auto run = [&](const char* tag, std::unique_ptr<ITextBufferCore> core){
    TextBuffer b; b.core = std::move(core); b.init_from_lines(lines);
    // head
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.insert_iters; ++i) b.insert_line(0, "x");
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_line head iters=" << cfg.insert_iters << " took " << dt.count() << "s\n";
    }
    // mid
    {
      int mid = std::max(0, b.line_count()/2);
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.insert_iters; ++i) b.insert_line(mid, "x");
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_line mid  iters=" << cfg.insert_iters << " took " << dt.count() << "s\n";
    }
    // tail
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.insert_iters; ++i) b.insert_line(b.line_count(), "x");
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_line tail iters=" << cfg.insert_iters << " took " << dt.count() << "s\n";
    }
  };
  run("[vector]  ", std::make_unique<VectorTextBufferCore>());
  run("[gap]     ", std::make_unique<GapTextBufferCore>());
  // piece table removed
  run("[rope]    ", std::make_unique<RopeTextBufferCore>());
}

static void bench_insert_lines(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  std::vector<std::string> block(cfg.block_size, std::string("blk"));
  auto run = [&](const char* tag, std::unique_ptr<ITextBufferCore> core){
    TextBuffer b; b.core = std::move(core); b.init_from_lines(lines);
    // head
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) b.insert_lines(0, block);
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_lines head iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
    // mid
    {
      int mid = std::max(0, b.line_count()/2);
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) b.insert_lines(mid, block);
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_lines mid  iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
    // tail
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) b.insert_lines(b.line_count(), block);
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " insert_lines tail iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
  };
  run("[vector]  ", std::make_unique<VectorTextBufferCore>());
  run("[gap]     ", std::make_unique<GapTextBufferCore>());
  // piece table removed
  run("[rope]    ", std::make_unique<RopeTextBufferCore>());
}

static void bench_erase_line(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  auto run = [&](const char* tag, std::unique_ptr<ITextBufferCore> core){
    TextBuffer b; b.core = std::move(core); b.init_from_lines(lines);
    // head
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.erase_iters; ++i) b.erase_line(0);
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_line  head iters=" << cfg.erase_iters << " took " << dt.count() << "s\n";
    }
    // mid
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.erase_iters; ++i) {
        int mid = std::max(0, b.line_count()/2 - 1);
        if (b.line_count() > 0) b.erase_line(mid);
      }
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_line  mid  iters=" << cfg.erase_iters << " took " << dt.count() << "s\n";
    }
    // tail
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.erase_iters; ++i) {
        if (b.line_count() > 0) b.erase_line(b.line_count()-1);
      }
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_line  tail iters=" << cfg.erase_iters << " took " << dt.count() << "s\n";
    }
  };
  run("[vector]  ", std::make_unique<VectorTextBufferCore>());
  run("[gap]     ", std::make_unique<GapTextBufferCore>());
  // piece table removed
  run("[rope]    ", std::make_unique<RopeTextBufferCore>());
}

static void bench_erase_lines(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  auto run = [&](const char* tag, std::unique_ptr<ITextBufferCore> core){
    TextBuffer b; b.core = std::move(core); b.init_from_lines(lines);
    // head
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) {
        int end = std::min(cfg.block_size, b.line_count());
        b.erase_lines(0, end);
      }
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_lines head iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
    // mid
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) {
        int mid = std::max(0, b.line_count()/2 - cfg.block_size/2);
        int end = std::min(b.line_count(), mid + cfg.block_size);
        if (end > mid) b.erase_lines(mid, end);
      }
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_lines mid  iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
    // tail
    {
      auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < cfg.block_iters; ++i) {
        int end = b.line_count();
        int start = std::max(0, end - cfg.block_size);
        if (end > start) b.erase_lines(start, end);
      }
      auto t1 = std::chrono::steady_clock::now();
      std::chrono::duration<double> dt = t1 - t0;
      std::cout << tag << " erase_lines tail iters=" << cfg.block_iters << " blk=" << cfg.block_size << " took " << dt.count() << "s\n";
    }
  };
  run("[vector]  ", std::make_unique<VectorTextBufferCore>());
  run("[gap]     ", std::make_unique<GapTextBufferCore>());
  // piece table removed
  run("[rope]    ", std::make_unique<RopeTextBufferCore>());
}

static void bench_replace_line(const BenchCfg& cfg) {
  auto lines = make_lines(cfg.N);
  std::mt19937 rng(54321);
  std::uniform_int_distribution<int> dist(0, cfg.N - 1);
  auto run = [&](const char* tag, std::unique_ptr<ITextBufferCore> core){
    TextBuffer b; b.core = std::move(core); b.init_from_lines(lines);
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < cfg.get_iters; ++i) {
      int r = dist(rng);
      b.replace_line(r, "rep");
    }
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> dt = t1 - t0;
    std::cout << tag << " replace_line iters=" << cfg.get_iters << " took " << dt.count() << "s\n";
  };
  run("[vector]  ", std::make_unique<VectorTextBufferCore>());
  run("[gap]     ", std::make_unique<GapTextBufferCore>());
  // piece table removed
  run("[rope]    ", std::make_unique<RopeTextBufferCore>());
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
