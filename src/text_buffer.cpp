#include "text_buffer.hpp"
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include "posix_fd.hpp"
#include <sys/stat.h>
#include "file_reader.hpp"
#include "config.hpp"
#include "config.hpp"

TextBuffer::TextBuffer() {}

std::string_view TextBuffer::backend_name() const {
  return core.get_name();
}

bool TextBuffer::empty() const { return line_count() == 0; }
int TextBuffer::line_count() const { return core.line_count(); }
std::string TextBuffer::line(int r) const { return core.get_line(r); }

void TextBuffer::ensure_not_empty() {
  if (line_count() == 0) core.insert_line(static_cast<size_t>(0), std::string());
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
  core.insert_line(static_cast<size_t>(row), s);
}


void TextBuffer::insert_lines(int row, const std::vector<std::string>& ss) {
  core.insert_lines(static_cast<size_t>(row), ss);
}


void TextBuffer::erase_line(int row) {
  core.erase_line(static_cast<size_t>(row));
  ensure_not_empty();
}

void TextBuffer::erase_lines(int start_row, int end_row) {
  core.erase_lines(static_cast<size_t>(start_row), static_cast<size_t>(end_row));
  ensure_not_empty();
}

void TextBuffer::replace_line(int row, const std::string& s) {
  core.replace_line(static_cast<size_t>(row), s);
}


TextBuffer TextBuffer::from_file(const std::filesystem::path& path, std::string& msg, bool& ok) {
  TextBuffer b;
  ok = true;
  std::vector<std::string> ls;
  if (!mmap_readlines(path, ls, msg)) {
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
  UniqueFd ufd(::open(tmp.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644));
  if (!ufd.valid()) {
    msg = std::string("write file failed: ") + tmp.string();
    return false;
  }
  int n = line_count();
  std::vector<char> buf(static_cast<size_t>(TB_WRITE_CHUNK_SIZE));
  size_t used = 0;
  auto flush_buf = [&](int fd) -> bool {
    const char* p = buf.data();
    size_t remain = used;
    while (remain > 0) {
      ssize_t w = ::write(fd, p, remain);
      if (w < 0) { msg = std::string("write file failed: ") + tmp.string(); return false; }
      p += w;
      remain -= static_cast<size_t>(w);
    }
    used = 0;
    return true;
  };
  auto write_span = [&](int fd, const char* p, size_t len) -> bool {
    size_t remain = len;
    const char* q = p;
    while (remain > 0) {
      ssize_t w = ::write(fd, q, remain);
      if (w < 0) { msg = std::string("write file failed: ") + tmp.string(); return false; }
      q += w;
      remain -= static_cast<size_t>(w);
    }
    return true;
  };
  for (int i = 0; i < n; ++i) {
    std::string s = line(i);
    bool need_nl = (i + 1 < n);
    if (s.size() + (need_nl ? 1u : 0u) > buf.size() - used) {
      if (used > 0) {
        if (!flush_buf(ufd.get())) return false;
      }
      if (s.size() >= buf.size()) {
        if (!write_span(ufd.get(), s.data(), s.size())) return false;
        if (need_nl) {
          char nl = '\n';
          if (!write_span(ufd.get(), &nl, 1)) return false;
        }
        continue;
      }
    }
    std::memcpy(buf.data() + used, s.data(), s.size());
    used += s.size();
    if (need_nl) {
      if (used == buf.size()) {
        if (!flush_buf(ufd.get())) return false;
      }
      if (used < buf.size()) {
        buf[used++] = '\n';
      } else {
        char nl = '\n';
        if (!write_span(ufd.get(), &nl, 1)) return false;
      }
    }
  }
  if (used > 0) {
    if (!flush_buf(ufd.get())) return false;
  }
#if defined(__APPLE__)
  if (::fsync(ufd.get()) != 0) { msg = std::string("write file failed: ") + tmp.string(); return false; }
#else
  if (::fdatasync(ufd.get()) != 0) { msg = std::string("write file failed: ") + tmp.string(); return false; }
#endif
  ufd.reset();
  std::error_code ec;
  std::filesystem::rename(tmp, path, ec);
  if (ec) { msg = std::string("write file failed: ") + path.string(); return false; }
  msg = std::string("saved file: ") + path.string();
  return true;
}
