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
#include <memory>
#include <string>
#include "i_text_buffer_core.hpp"
#include "config.hpp"
#if TB_BACKEND == TB_BACKEND_GAP
#include "gap_text_buffer_core.hpp"
#elif TB_BACKEND == TB_BACKEND_ROPE
#include "rope_text_buffer_core.hpp"
#else
#include "vector_text_buffer_core.hpp"
#endif

class TextBuffer {
public:
  TextBuffer();
  // 使用编译期选择的具体后端类型（CRTP 非虚调用）
#if TB_BACKEND == TB_BACKEND_GAP
  GapTextBufferCore core;
#elif TB_BACKEND == TB_BACKEND_ROPE
  RopeTextBufferCore core;
#else
  VectorTextBufferCore core;
#endif

  std::string_view backend_name() const;
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
