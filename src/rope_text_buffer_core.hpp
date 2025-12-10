#pragma once
#include <string>
#include <vector>
#include <memory>
#include <string_view>
#include <span>
#include <future>
#include "i_text_buffer_core.hpp"

class RopeTextBufferCore : public TextBufferCoreCRTP<RopeTextBufferCore> {
public:
  static constexpr std::string_view get_name_sv() { return "rope"; }
  void init_from_lines(const std::vector<std::string>& lines);
  int line_count() const;
  std::string get_line(int r) const;

  void insert_line(size_t row, const std::string& s);
  void insert_line(size_t row, std::string_view s);
  void insert_lines(size_t row, const std::vector<std::string>& ss);
  void insert_lines(size_t row, std::span<const std::string> ss);
  void erase_line(size_t row);
  void erase_lines(size_t start_row, size_t end_row); // end_row exclusive
  void replace_line(size_t row, const std::string& s);
  void replace_line(size_t row, std::string_view s);

  /*forward to CRTP impl*/
  void do_init_from_lines(const std::vector<std::string>& lines) { init_from_lines(lines); }
  int do_line_count() const { return line_count(); }
  std::string do_get_line(int r) const { return get_line(r); }
  void do_insert_line(size_t row, const std::string& s) { insert_line(row, s); }
  void do_insert_line(size_t row, std::string_view s) { insert_line(row, s); }
  void do_insert_lines(size_t row, const std::vector<std::string>& ss) { insert_lines(row, ss); }
  void do_insert_lines(size_t row, std::span<const std::string> ss) { insert_lines(row, ss); }
  void do_erase_line(size_t row) { erase_line(row); }
  void do_erase_lines(size_t start_row, size_t end_row) { erase_lines(start_row, end_row); }
  void do_replace_line(size_t row, const std::string& s) { replace_line(row, s); }
  void do_replace_line(size_t row, std::string_view s) { replace_line(row, s); }

private:
  struct Node {
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
    std::vector<std::string> lines; /* non-empty only for leaves */
    size_t lines_count = 0;         /* aggregated number of lines */
    int height = 1;                 /* AVL height */
  };
  std::unique_ptr<Node> root_;

  static size_t count_lines(const Node* n) { return n ? n->lines_count : 0; }
  static int node_height(const Node* n) { return n ? n->height : 0; }
  static int balance_factor(const Node* n) { return n ? (node_height(n->left.get()) - node_height(n->right.get())) : 0; }
  static void recalc(Node* n);
  static std::unique_ptr<Node> rotate_left(std::unique_ptr<Node> x);
  static std::unique_ptr<Node> rotate_right(std::unique_ptr<Node> y);
  static std::unique_ptr<Node> balance(std::unique_ptr<Node> n);

  static std::unique_ptr<Node> make_leaf(std::vector<std::string>&& lines);
  static std::unique_ptr<Node> concat(std::unique_ptr<Node> a, std::unique_ptr<Node> b);
  static std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>> split(std::unique_ptr<Node> n, size_t k);
  static std::unique_ptr<Node> build_balanced(const std::vector<std::string>& lines, size_t l, size_t r);
  static std::unique_ptr<Node> build_balanced_parallel(const std::vector<std::string>& lines, size_t l, size_t r);
  static std::string get_line_at(const Node* n, size_t r);

  static std::unique_ptr<Node> normalize_node(std::unique_ptr<Node> n);
};

static_assert(TextBufferCoreCRTPConcept<RopeTextBufferCore>, "Rope backend must satisfy CRTP concept");
