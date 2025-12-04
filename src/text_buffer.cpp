#include "text_buffer.hpp"
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "file_reader.hpp"
#include "config.hpp"
#include "posix_fd.hpp"
#if USE_GAP
#include "gap_text_buffer_core.hpp"
#else
#include "vector_text_buffer_core.hpp"
#endif

TextBuffer::TextBuffer() {
#if USE_GAP
  core = std::make_unique<GapTextBufferCore>();
#else
  core = std::make_unique<VectorTextBufferCore>();
#endif
}

bool TextBuffer::empty() const { return line_count() == 0; }
int TextBuffer::line_count() const { return core->line_count(); }
std::string TextBuffer::line(int r) const { return core->get_line(r); }

void TextBuffer::ensure_not_empty() {
  if (line_count() == 0) core->insert_line(static_cast<size_t>(0), std::string());
}

void TextBuffer::init_from_lines(const std::vector<std::string>& src) {
  core->init_from_lines(src);
  ensure_not_empty();
}

void TextBuffer::init_from_lines(std::vector<std::string>&& src) {
  core->init_from_lines(src);
  ensure_not_empty();
}

void TextBuffer::insert_line(int row, const std::string& s) {
  core->insert_line(static_cast<size_t>(row), s);
}


void TextBuffer::insert_lines(int row, const std::vector<std::string>& ss) {
  core->insert_lines(static_cast<size_t>(row), ss);
}


void TextBuffer::erase_line(int row) {
  core->erase_line(static_cast<size_t>(row));
  ensure_not_empty();
}

void TextBuffer::erase_lines(int start_row, int end_row) {
  core->erase_lines(static_cast<size_t>(start_row), static_cast<size_t>(end_row));
  ensure_not_empty();
}

void TextBuffer::replace_line(int row, const std::string& s) {
  core->replace_line(static_cast<size_t>(row), s);
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
  std::string out;
  {
    int n = line_count();
    size_t total = 0;
    for (int i = 0; i < n; ++i) {
      total += line(i).size();
    }
    if (n > 1) {
      total += static_cast<size_t>(n - 1);
    }
    out.reserve(total);
    for (int i = 0; i < n; ++i) {
      std::string s = line(i);
      out.append(s);
      if (i + 1 < n) {
        out.push_back('\n');
      }
    }
  }
  const char* p = out.data();
  size_t remain = out.size();
  while (remain > 0) {
    ssize_t w = ::write(ufd.get(), p, remain);
    if (w < 0) { msg = std::string("write file failed: ") + tmp.string(); return false; }
    p += w;
    remain -= static_cast<size_t>(w);
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
