#include "text_buffer.hpp"
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "file_reader.hpp"

bool TextBuffer::empty() const { return lines.empty(); }
int TextBuffer::line_count() const { return (int)lines.size(); }
const std::string& TextBuffer::line(int r) const { return lines[r]; }
std::string& TextBuffer::line(int r) { return lines[r]; }

void TextBuffer::ensure_not_empty() {
  if (lines.empty()) lines.emplace_back("");
}

TextBuffer TextBuffer::from_file(const std::filesystem::path& path, std::string& msg, bool& ok) {
  TextBuffer b;
  ok = true;
  std::vector<std::string> lines;
  if (!mmapReadLines(path, lines, msg)) {
    ok = false;
    b.ensure_not_empty();
    return b;
  }
  b.lines = std::move(lines);
  if (b.lines.empty()) b.lines.emplace_back("");
  return b;
}

bool TextBuffer::write_file(const std::filesystem::path& path, std::string& msg) const {
  std::filesystem::path tmp = path;
  tmp += ".tmp";
  int fd = ::open(tmp.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    msg = std::string("write file failed: ") + tmp.string();
    return false;
  }
  size_t total = 0;
  if (!lines.empty()) {
    for (const auto& s : lines) total += s.size();
    total += (lines.size() - 1);
  }
  std::string out;
  out.reserve(total);
  for (size_t i = 0; i < lines.size(); ++i) {
    out.append(lines[i]);
    if (i + 1 < lines.size()) out.push_back('\n');
  }
  const char* p = out.data();
  size_t remain = out.size();
  while (remain > 0) {
    ssize_t w = ::write(fd, p, remain);
    if (w < 0) { ::close(fd); msg = std::string("write file failed: ") + tmp.string(); return false; }
    p += w; remain -= static_cast<size_t>(w);
  }
#if defined(__APPLE__)
  if (::fsync(fd) != 0) { ::close(fd); msg = std::string("write file failed: ") + tmp.string(); return false; }
#else
  if (::fdatasync(fd) != 0) { ::close(fd); msg = std::string("write file failed: ") + tmp.string(); return false; }
#endif
  ::close(fd);
  std::error_code ec;
  std::filesystem::rename(tmp, path, ec);
  if (ec) { msg = std::string("write file failed: ") + path.string(); return false; }
  msg = std::string("saved file: ") + path.string();
  return true;
}
