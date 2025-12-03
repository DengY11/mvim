#include "file_reader.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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

  ::munmap(mem, n);
  ::close(fd);
  if (out_lines.empty()) out_lines.emplace_back("");
  msg = std::string("opened file: ") + path.string();
  return true;
}
