#pragma once
/*
 * FileReader
 *
 * 作用：通过 mmap 高效读取文件并切分为行，归一化 CRLF。
 * 用法：mmapReadLines(path, out_lines, msg)；失败时返回 false 并附带消息。
 */
#include <vector>
#include <string>
#include <filesystem>

bool mmapReadLines(const std::filesystem::path& path,
                   std::vector<std::string>& out_lines,
                   std::string& msg);

