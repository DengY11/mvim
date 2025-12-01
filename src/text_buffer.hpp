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

class TextBuffer {
public:
  std::vector<std::string> lines;

  bool empty() const;
  int line_count() const;
  const std::string& line(int r) const;
  std::string& line(int r);
  void ensure_not_empty();

  static TextBuffer from_file(const std::filesystem::path& path, std::string& msg, bool& ok);
  bool write_file(const std::filesystem::path& path, std::string& msg) const;
};
