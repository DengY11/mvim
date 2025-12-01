#pragma once
/*
 * FileReader
 *
 * Purpose: efficiently read file via mmap and split into lines; normalize CRLF.
 * Usage: mmapReadLines(path, out_lines, msg); returns false with msg on failure.
 */
#include <vector>
#include <string>
#include <filesystem>

bool mmapReadLines(const std::filesystem::path& path,
                   std::vector<std::string>& out_lines,
                   std::string& msg);
