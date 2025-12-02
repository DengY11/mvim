#include "gap_buffer.hpp"

void GapBuffer::clear() { buf.clear(); gap_start = gap_end = 0; }

size_t GapBuffer::length() const { return buf.size() - (gap_end - gap_start); }

void GapBuffer::init_from_lines(const std::vector<std::string>& lines) {
  size_t total = 0;
  if (!lines.empty()) { for (const auto& s : lines) total += s.size(); total += lines.size() - 1; }
  buf.resize(total);
  size_t k = 0;
  for (size_t i = 0; i < lines.size(); ++i) {
    const std::string& s = lines[i];
    for (size_t j = 0; j < s.size(); ++j) buf[k++] = s[j];
    if (i + 1 < lines.size()) buf[k++] = '\n';
  }
  gap_start = gap_end = k;
}

void GapBuffer::ensure_gap(size_t need) {
  size_t avail = (gap_end - gap_start);
  if (avail >= need) return;
  size_t grow = need - avail;
  size_t new_size = buf.size() + grow + grow;
  std::vector<char> nb;
  nb.resize(new_size);
  size_t left = gap_start;
  for (size_t i = 0; i < left; ++i) nb[i] = buf[i];
  size_t right = buf.size() - gap_end;
  size_t ngs = left;
  size_t nge = ngs + avail + grow + grow;
  for (size_t i = 0; i < right; ++i) nb[nge + i] = buf[gap_end + i];
  buf.swap(nb);
  gap_start = ngs;
  gap_end = nge;
}

void GapBuffer::move_gap_to(size_t pos) {
  if (pos == gap_start) return;
  if (pos < gap_start) {
    size_t delta = gap_start - pos;
    for (size_t i = 0; i < delta; ++i) buf[gap_end - 1 - i] = buf[gap_start - 1 - i];
    gap_start -= delta; gap_end -= delta;
  } else {
    size_t delta = pos - gap_start;
    for (size_t i = 0; i < delta; ++i) buf[gap_start + i] = buf[gap_end + i];
    gap_start += delta; gap_end += delta;
  }
}

void GapBuffer::insert_text(const std::string& text) {
  ensure_gap(text.size());
  for (size_t i = 0; i < text.size(); ++i) buf[gap_start + i] = text[i];
  gap_start += text.size();
}

void GapBuffer::erase_range(size_t pos, size_t len) {
  move_gap_to(pos);
  gap_end += len;
}

std::string GapBuffer::slice(size_t pos, size_t len) const {
  std::string out; out.resize(len);
  for (size_t i = 0; i < len; ++i) {
    size_t p = pos + i;
    if (p < gap_start) out[i] = buf[p]; else out[i] = buf[p + (gap_end - gap_start)];
  }
  return out;
}
