#pragma once
/*
 * TextBuffer
 *
 * Purpose: line-based text buffer supporting line/char ops and file I/O.
 * Feature: safe writes (write .tmp → fsync/fdatasync → atomic rename).
 * Note: keep API simple (line/insert/delete/split), future Gap/Rope swap.
 */
#include <string>
#include <vector>
#include <filesystem>
#include "config.hpp"
#if USE_GAP
#include "text_buffer_core.hpp" // Gap backend
#else
#include "vector_text_buffer_core.hpp" // Vector backend
#endif

class TextBuffer {
public:
#if USE_GAP
  TextBufferCore core;
#else
  VectorTextBufferCore core;
#endif
  bool empty() const;
  int line_count() const;
  std::string line(int r) const;
  void ensure_not_empty();

  void init_from_lines(const std::vector<std::string>& lines);
  void init_from_lines(std::vector<std::string>&& lines);

  void insert_line(int row, const std::string& s);
  void insert_lines(int row, const std::vector<std::string>& ss);
  void erase_line(int row);
  void erase_lines(int start_row, int end_row);
  void replace_line(int row, const std::string& s);

  static TextBuffer from_file(const std::filesystem::path& path, std::string& msg, bool& ok);
  bool write_file(const std::filesystem::path& path, std::string& msg) const;
};
