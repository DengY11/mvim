#pragma once
#include <string>
#include <vector>
#include <memory>
#include <string_view>
#include <span>
#include <future>
#include "i_text_buffer_core.hpp"

class RopeTextBufferCore : public ITextBufferCore {
public:
  std::string_view get_name() const override { return "rope"; }
  void init_from_lines(const std::vector<std::string>& lines) override;
  int line_count() const override;
  std::string get_line(int r) const override;

  void insert_line(size_t row, const std::string& s) override;
  void insert_line(size_t row, std::string_view s) override;
  void insert_lines(size_t row, const std::vector<std::string>& ss) override;
  void insert_lines(size_t row, std::span<const std::string> ss) override;
  void erase_line(size_t row) override;
  void erase_lines(size_t start_row, size_t end_row) override; // end_row exclusive
  void replace_line(size_t row, const std::string& s) override;
  void replace_line(size_t row, std::string_view s) override;

private:
  struct Node {
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
    std::vector<std::string> lines; // non-empty only for leaves
    size_t lines_count = 0;         // aggregated number of lines
  };
  std::unique_ptr<Node> root_;

  static size_t count_lines(const Node* n) { return n ? n->lines_count : 0; }
  static void recalc(Node* n);
  static std::unique_ptr<Node> make_leaf(std::vector<std::string>&& lines);
  static std::unique_ptr<Node> concat(std::unique_ptr<Node> a, std::unique_ptr<Node> b);
  static std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>> split(std::unique_ptr<Node> n, size_t k);
  static std::unique_ptr<Node> build_balanced(const std::vector<std::string>& lines, size_t l, size_t r);
  static std::unique_ptr<Node> build_balanced_parallel(const std::vector<std::string>& lines, size_t l, size_t r);
  static std::string get_line_at(const Node* n, size_t r);
};
