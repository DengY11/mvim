#pragma once
/*
 * TextBuffer
 *
 * 作用：基于行的文本缓冲，提供行/字符操作与文件读写。
 * 特性：默认安全写入（写入 .tmp → fsync → 原子重命名），避免数据损坏。
 * 关注：接口保持简单（line/insert/delete/split），便于后续替换为 Gap/Rope。
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
