#include "text_buffer.hpp"
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "file_reader.hpp"
#include "config.hpp"
#if USE_GAP
#include "text_buffer_core.hpp"
#endif

bool TextBuffer::empty() const { return line_count() == 0; }
int TextBuffer::line_count() const { return core.line_count(); }
std::string TextBuffer::line(int r) const { return core.get_line(r); }

void TextBuffer::ensure_not_empty() {
  if (line_count() == 0) core.insert_line(0, std::string());
}

void TextBuffer::init_from_lines(const std::vector<std::string>& src) {
  core.init_from_lines(src);
  ensure_not_empty();
}

void TextBuffer::init_from_lines(std::vector<std::string>&& src) {
  core.init_from_lines(src);
  ensure_not_empty();
}

void TextBuffer::insert_line(int row, const std::string& s) {
  core.insert_line(row, s);
}

void TextBuffer::insert_lines(int row, const std::vector<std::string>& ss) {
  core.insert_lines(row, ss);
}

void TextBuffer::erase_line(int row) {
  core.erase_line(row);
  ensure_not_empty();
}

void TextBuffer::erase_lines(int start_row, int end_row) {
  core.erase_lines(start_row, end_row);
  ensure_not_empty();
}

void TextBuffer::replace_line(int row, const std::string& s) {
  core.replace_line(row, s);
}

TextBuffer TextBuffer::from_file(const std::filesystem::path& path, std::string& msg, bool& ok) {
  TextBuffer b;
  ok = true;
  std::vector<std::string> ls;
  if (!mmapReadLines(path, ls, msg)) {
    ok = false;
    b.ensure_not_empty();
    return b;
  }
  b.init_from_lines(std::move(ls));
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
  std::string out;
  {
    int n = line_count();
    size_t total = 0; for (int i = 0; i < n; ++i) total += line(i).size(); if (n > 1) total += (size_t)(n - 1);
    out.reserve(total);
    for (int i = 0; i < n; ++i) { std::string s = line(i); out.append(s); if (i + 1 < n) out.push_back('\n'); }
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
