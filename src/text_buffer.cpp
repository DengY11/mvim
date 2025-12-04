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
  std::string out;
  int n = line_count();
  if (n <= 0) {
    out = std::string();
  } else {
    std::vector<size_t> lens(static_cast<size_t>(n));
    unsigned hw = std::thread::hardware_concurrency(); if (hw == 0) hw = 4;
    if (n >= 1024 && hw > 1) {
      unsigned t = std::min<unsigned>(hw, static_cast<unsigned>(n / 1024)); t = std::max(t, 2u);
      size_t per = (static_cast<size_t>(n) + t - 1) / t;
      std::vector<std::future<void>> futs;
      for (unsigned k = 0; k < t; ++k) {
        size_t i0 = k * per; size_t i1 = std::min(static_cast<size_t>(n), (k + 1) * per);
        if (i0 >= i1) break;
        futs.emplace_back(std::async(std::launch::async, [&, i0, i1]{
          for (size_t i = i0; i < i1; ++i) lens[i] = line(static_cast<int>(i)).size();
        }));
      }
      for (auto& f : futs) f.get();
    } else {
      for (int i = 0; i < n; ++i) lens[static_cast<size_t>(i)] = line(i).size();
    }
    std::vector<size_t> pos(static_cast<size_t>(n));
    size_t total = 0;
    for (int i = 0; i < n; ++i) {
      pos[static_cast<size_t>(i)] = total;
      total += lens[static_cast<size_t>(i)];
      if (i + 1 < n) total += 1; // newline
    }
    out.resize(total);
    char* out_data = out.data();
    // 并行拷贝内容
    if (n >= 1024 && hw > 1) {
      unsigned t = std::min<unsigned>(hw, static_cast<unsigned>(n / 1024)); t = std::max(t, 2u);
      size_t per = (static_cast<size_t>(n) + t - 1) / t;
      std::vector<std::future<void>> futs;
      for (unsigned k = 0; k < t; ++k) {
        size_t i0 = k * per; size_t i1 = std::min(static_cast<size_t>(n), (k + 1) * per);
        if (i0 >= i1) break;
        futs.emplace_back(std::async(std::launch::async, [&, i0, i1]{
          for (size_t i = i0; i < i1; ++i) {
            std::string s = line(static_cast<int>(i));
            size_t p = pos[i];
            std::memcpy(out_data + p, s.data(), s.size());
            if (i + 1 < static_cast<size_t>(n)) out_data[p + s.size()] = '\n';
          }
        }));
      }
      for (auto& f : futs) f.get();
    } else {
      for (int i = 0; i < n; ++i) {
        std::string s = line(i);
        size_t p0 = pos[static_cast<size_t>(i)];
        std::memcpy(out_data + p0, s.data(), s.size());
        if (i + 1 < n) out_data[p0 + s.size()] = '\n';
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
