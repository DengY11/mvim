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

bool TextBuffer::empty() const { return lines.empty(); }
int TextBuffer::line_count() const { return (int)lines.size(); }
const std::string& TextBuffer::line(int r) const { return lines[r]; }
std::string& TextBuffer::line(int r) { return lines[r]; }

void TextBuffer::ensure_not_empty() {
  if (lines.empty()) lines.emplace_back("");
}

void TextBuffer::insert_line(int row, const std::string& s) {
  if (row < 0) row = 0;
  if (row > (int)lines.size()) row = (int)lines.size();
  lines.insert(lines.begin() + row, s);
}

void TextBuffer::insert_lines(int row, const std::vector<std::string>& ss) {
  if (row < 0) row = 0;
  if (row > (int)lines.size()) row = (int)lines.size();
  lines.insert(lines.begin() + row, ss.begin(), ss.end());
}

void TextBuffer::erase_line(int row) {
  if (row < 0 || row >= (int)lines.size()) return;
  lines.erase(lines.begin() + row);
  if (lines.empty()) lines.emplace_back("");
}

void TextBuffer::erase_lines(int start_row, int end_row) {
  if (start_row < 0) start_row = 0;
  if (end_row < start_row) end_row = start_row;
  if (start_row >= (int)lines.size()) return;
  if (end_row > (int)lines.size()) end_row = (int)lines.size();
  lines.erase(lines.begin() + start_row, lines.begin() + end_row);
  if (lines.empty()) lines.emplace_back("");
}

void TextBuffer::replace_line(int row, const std::string& s) {
  if (row < 0 || row >= (int)lines.size()) return;
  lines[row] = s;
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
#if USE_GAP
  b.core.init_from_lines(ls);
  b.lines = std::move(ls);
#else
  b.lines = std::move(ls);
#endif
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
  std::string out;
#if USE_GAP
  {
    size_t total = core.gb.length();
    out.resize(total);
    size_t gs = core.gb.gap_start;
    size_t ge = core.gb.gap_end;
    size_t k = 0;
    for (size_t i = 0; i < core.gb.buf.size(); ++i) {
      if (i >= gs && i < ge) continue;
      out[k++] = core.gb.buf[i];
    }
  }
#else
  {
    size_t total = 0; if (!lines.empty()) { for (const auto& s : lines) total += s.size(); total += (lines.size() - 1); }
    out.reserve(total);
    for (size_t i = 0; i < lines.size(); ++i) { out.append(lines[i]); if (i + 1 < lines.size()) out.push_back('\n'); }
  }
#endif
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
