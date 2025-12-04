#include "file_reader.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <future>
#include <algorithm>

bool mmap_readlines(const std::filesystem::path& path,
                   std::vector<std::string>& out_lines,
                   std::string& msg) {
  out_lines.clear();
  int fd = ::open(path.string().c_str(), O_RDONLY);
  if (fd < 0) { msg = std::string("can not open file: ") + path.string(); return false; }
  struct stat st{};
  if (::fstat(fd, &st) != 0) { ::close(fd); msg = std::string("can not read file stat: ") + path.string(); return false; }
  size_t n = static_cast<size_t>(st.st_size);
  if (n == 0) { ::close(fd); out_lines.emplace_back(""); msg = std::string("opened") + path.string(); return true; }
  void* mem = ::mmap(nullptr, n, PROT_READ, MAP_PRIVATE, fd, 0);
  if (mem == MAP_FAILED) { ::close(fd); msg = std::string("can not mmap file: ") + path.string(); return false; }
  const char* data = static_cast<const char*>(mem);
  (void)::madvise(const_cast<char*>(data), n, MADV_SEQUENTIAL);

  unsigned hw = std::thread::hardware_concurrency();
  if (hw == 0) hw = 4;
  const size_t min_parallel_size = 1 << 20; 
  if (n < min_parallel_size || hw == 1) {
    size_t start = 0;
    for (size_t i = 0; i < n; ++i) {
      if (data[i] == '\n') {
        size_t end = i;
        if (end > start && data[end - 1] == '\r') end--;
        out_lines.emplace_back(data + start, end - start);
        start = i + 1;
      }
    }
    if (start <= n) {
      size_t end = n;
      if (end > start && data[end - 1] == '\r') end--;
      if (end >= start) out_lines.emplace_back(data + start, end - start);
    }
  } else {
    unsigned threads = std::min<unsigned>(hw, static_cast<unsigned>(n / min_parallel_size));
    threads = std::max(threads, 2u);
    std::vector<std::vector<size_t>> newline_pos(threads);
    std::vector<std::future<void>> futs;
    size_t chunk = n / threads;
    for (unsigned t = 0; t < threads; ++t) {
      size_t s = t * chunk;
      size_t e = (t + 1 == threads) ? n : (t + 1) * chunk;
      futs.emplace_back(std::async(std::launch::async, [&, s, e, t]{
        auto& vec = newline_pos[t];
        vec.reserve((e - s) / 64 + 1);
        for (size_t i = s; i < e; ++i) {
          if (data[i] == '\n') vec.push_back(i);
        }
      }));
    }
    for (auto& f : futs) f.get();
    size_t total_nl = 0; for (const auto& v : newline_pos) total_nl += v.size();
    std::vector<size_t> nl; nl.reserve(total_nl);
    for (unsigned t = 0; t < threads; ++t) {
      nl.insert(nl.end(), newline_pos[t].begin(), newline_pos[t].end());
    }
    std::vector<std::pair<size_t,size_t>> ranges; ranges.reserve(nl.size() + 1);
    size_t start = 0;
    for (size_t pos : nl) {
      size_t end = pos;
      if (end > start && data[end - 1] == '\r') end--;
      ranges.emplace_back(start, end);
      start = pos + 1;
    }
    if (start <= n) {
      size_t end = n;
      if (end > start && data[end - 1] == '\r') end--;
      if (end >= start) ranges.emplace_back(start, end);
    }
    out_lines.resize(ranges.size());
    unsigned tcopy = std::min<unsigned>(hw, static_cast<unsigned>(ranges.size()));
    if (tcopy <= 1 || ranges.size() < 1024) {
      for (size_t i = 0; i < ranges.size(); ++i) {
        auto [s, e] = ranges[i];
        out_lines[i] = std::string(data + s, e - s);
      }
    } else {
      size_t per = (ranges.size() + tcopy - 1) / tcopy;
      std::vector<std::future<void>> f2;
      for (unsigned t = 0; t < tcopy; ++t) {
        size_t i0 = t * per;
        size_t i1 = std::min(ranges.size(), (t + 1) * per);
        if (i0 >= i1) break;
        f2.emplace_back(std::async(std::launch::async, [&, i0, i1]{
          for (size_t i = i0; i < i1; ++i) {
            auto [s, e] = ranges[i];
            out_lines[i] = std::string(data + s, e - s);
          }
        }));
      }
      for (auto& f : f2) f.get();
    }
  }

  ::munmap(mem, n);
  ::close(fd);
  if (out_lines.empty()) out_lines.emplace_back("");
  msg = std::string("opened file: ") + path.string();
  return true;
}
