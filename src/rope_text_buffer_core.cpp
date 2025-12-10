#include "rope_text_buffer_core.hpp"
#include <algorithm>
#include <numeric>

static constexpr size_t LEAF_MAX_LINES = 128;

void RopeTextBufferCore::recalc(Node* n) {
  if (!n) return;
  size_t l = count_lines(n->left.get());
  size_t r = count_lines(n->right.get());
  size_t self = n->lines.size();
  n->lines_count = l + r + self;
  n->height = 1 + std::max(node_height(n->left.get()), node_height(n->right.get()));
}

std::unique_ptr<RopeTextBufferCore::Node> RopeTextBufferCore::rotate_left(std::unique_ptr<Node> x) {
  auto y = std::move(x->right);
  auto T2 = std::move(y->left);
  y->left = std::move(x);
  y->left->right = std::move(T2);
  recalc(y->left.get());
  recalc(y.get());
  return y;
}

std::unique_ptr<RopeTextBufferCore::Node> RopeTextBufferCore::rotate_right(std::unique_ptr<Node> y) {
  auto x = std::move(y->left);
  auto T2 = std::move(x->right);
  x->right = std::move(y);
  x->right->left = std::move(T2);
  recalc(x->right.get());
  recalc(x.get());
  return x;
}

std::unique_ptr<RopeTextBufferCore::Node> RopeTextBufferCore::balance(std::unique_ptr<Node> n) {
  if (!n) return n;
  recalc(n.get());
  int bf = balance_factor(n.get());
  if (bf > 1) { // left heavy
    if (balance_factor(n->left.get()) < 0) {
      n->left = rotate_left(std::move(n->left));
    }
    return rotate_right(std::move(n));
  } else if (bf < -1) { // right heavy
    if (balance_factor(n->right.get()) > 0) {
      n->right = rotate_right(std::move(n->right));
    }
    return rotate_left(std::move(n));
  }
  return n;
}

std::unique_ptr<RopeTextBufferCore::Node> RopeTextBufferCore::make_leaf(std::vector<std::string>&& lines) {
  auto n = std::make_unique<Node>();
  n->lines = std::move(lines);
  recalc(n.get());
  return n;
}

std::unique_ptr<RopeTextBufferCore::Node> RopeTextBufferCore::concat(std::unique_ptr<Node> a, std::unique_ptr<Node> b) {
  if (!a) return b;
  if (!b) return a;
  auto p = std::make_unique<Node>();
  p->left = std::move(a);
  p->right = std::move(b);
  recalc(p.get());
  return balance(std::move(p));
}

std::pair<std::unique_ptr<RopeTextBufferCore::Node>, std::unique_ptr<RopeTextBufferCore::Node>>
RopeTextBufferCore::split(std::unique_ptr<Node> n, size_t k) {
  if (!n) return {nullptr, nullptr};
  size_t left_count = count_lines(n->left.get());
  if (k < left_count) {
    auto [a, b] = split(std::move(n->left), k);
    n->left = std::move(b);
    recalc(n.get());
    return {std::move(a), balance(std::move(n))};
  }
  k -= left_count;
  size_t self_count = n->lines.size();
  if (k < self_count) {
    std::vector<std::string> left_lines;
    std::vector<std::string> right_lines;
    left_lines.reserve(k);
    right_lines.reserve(self_count - k);
    for (size_t i = 0; i < self_count; ++i) {
      if (i < k) left_lines.push_back(n->lines[i]); else right_lines.push_back(n->lines[i]);
    }
    auto a = make_leaf(std::move(left_lines));
    auto rest = std::make_unique<Node>();
    rest->right = std::move(n->right);
    rest->left = make_leaf(std::move(right_lines));
    recalc(rest.get());
    return {std::move(a), balance(std::move(rest))};
  }
  k -= self_count;
  auto [a, b] = split(std::move(n->right), k);
  n->right = std::move(a);
  recalc(n.get());
  return {balance(std::move(n)), std::move(b)};
}

std::unique_ptr<RopeTextBufferCore::Node>
RopeTextBufferCore::build_balanced(const std::vector<std::string>& lines, size_t l, size_t r) {
  size_t len = r - l;
  if (len == 0) return nullptr;
  if (len <= 128) {
    std::vector<std::string> leaf;
    leaf.reserve(len);
    for (size_t i = l; i < r; ++i) leaf.push_back(lines[i]);
    return make_leaf(std::move(leaf));
  }
  size_t mid = l + len / 2;
  auto left = build_balanced(lines, l, mid);
  auto right = build_balanced(lines, mid, r);
  return concat(std::move(left), std::move(right));
}

std::unique_ptr<RopeTextBufferCore::Node>
RopeTextBufferCore::build_balanced_parallel(const std::vector<std::string>& lines, size_t l, size_t r) {
  size_t len = r - l;
  if (len <= 4096) return build_balanced(lines, l, r);
  size_t mid = l + len / 2;
  auto fut_left = std::async(std::launch::async, [&]{ return build_balanced(lines, l, mid); });
  auto right = build_balanced(lines, mid, r);
  auto left = fut_left.get();
  return concat(std::move(left), std::move(right));
}

std::string RopeTextBufferCore::get_line_at(const Node* n, size_t r) {
  const Node* cur = n;
  size_t idx = r;
  while (cur) {
    size_t lc = count_lines(cur->left.get());
    if (idx < lc) { cur = cur->left.get(); continue; }
    idx -= lc;
    size_t self = cur->lines.size();
    if (idx < self) { return cur->lines[idx]; }
    idx -= self;
    cur = cur->right.get();
  }
  return std::string();
}


std::unique_ptr<RopeTextBufferCore::Node>
RopeTextBufferCore::normalize_node(std::unique_ptr<Node> n) {
  if (!n) return nullptr;
  if (n->left.get() == nullptr && n->right.get() == nullptr) {
    size_t sz = n->lines.size();
    if (sz <= LEAF_MAX_LINES) { recalc(n.get()); return n; }
    size_t mid = sz / 2;
    std::vector<std::string> left_lines; left_lines.reserve(mid);
    std::vector<std::string> right_lines; right_lines.reserve(sz - mid);
    for (size_t i = 0; i < sz; ++i) {
      if (i < mid) left_lines.push_back(n->lines[i]); else right_lines.push_back(n->lines[i]);
    }
    auto L = make_leaf(std::move(left_lines));
    auto R = make_leaf(std::move(right_lines));
    return balance(concat(std::move(L), std::move(R)));
  }

  n->left = normalize_node(std::move(n->left));
  n->right = normalize_node(std::move(n->right));

  if (n->left && n->right && n->left->left.get() == nullptr && n->left->right.get() == nullptr && n->right->left.get() == nullptr && n->right->right.get() == nullptr) {
    size_t lsz = n->left->lines.size();
    size_t rsz = n->right->lines.size();
    if (lsz + rsz <= LEAF_MAX_LINES) {
      std::vector<std::string> merged; merged.reserve(lsz + rsz);
      for (auto& s : n->left->lines) merged.push_back(std::move(s));
      for (auto& s : n->right->lines) merged.push_back(std::move(s));
      auto leaf = make_leaf(std::move(merged));
      return leaf;
    }
  }

  recalc(n.get());
  return balance(std::move(n));
}

void RopeTextBufferCore::init_from_lines(const std::vector<std::string>& lines) {
  if (lines.empty()) { root_.reset(); return; }
  root_ = build_balanced_parallel(lines, 0, lines.size());
  root_ = normalize_node(std::move(root_));
}

int RopeTextBufferCore::line_count() const { return static_cast<int>(count_lines(root_.get())); }

std::string RopeTextBufferCore::get_line(int r) const {
  if (r < 0) return std::string();
  size_t rr = static_cast<size_t>(r);
  if (rr >= count_lines(root_.get())) return std::string();
  return get_line_at(root_.get(), rr);
}

void RopeTextBufferCore::insert_line(size_t row, const std::string& s) { insert_line(row, std::string_view(s)); }

void RopeTextBufferCore::insert_line(size_t row, std::string_view s) {
  size_t L = count_lines(root_.get()); if (row > L) row = L;
  auto [A, B] = split(std::move(root_), row);
  std::vector<std::string> single{std::string(s)};
  auto M = make_leaf(std::move(single));
  root_ = concat(concat(std::move(A), std::move(M)), std::move(B));
  root_ = normalize_node(std::move(root_));
}

void RopeTextBufferCore::insert_lines(size_t row, const std::vector<std::string>& ss) {
  insert_lines(row, std::span<const std::string>(ss.begin(), ss.end()));
}

void RopeTextBufferCore::insert_lines(size_t row, std::span<const std::string> ss) {
  if (ss.empty()) return;
  size_t L = count_lines(root_.get()); if (row > L) row = L;
  auto [A, B] = split(std::move(root_), row);
  // build M in parallel if large
  std::vector<std::string> temp; temp.reserve(ss.size()); for (const auto& s : ss) temp.push_back(s);
  auto M = (ss.size() >= 4096) ? build_balanced_parallel(temp, 0, temp.size()) : build_balanced(temp, 0, temp.size());
  root_ = concat(concat(std::move(A), std::move(M)), std::move(B));
  root_ = normalize_node(std::move(root_));
}

void RopeTextBufferCore::erase_line(size_t row) {
  size_t L = count_lines(root_.get()); if (row >= L) return;
  auto [A, B] = split(std::move(root_), row);
  auto [M, C] = split(std::move(B), 1);
  root_ = concat(std::move(A), std::move(C));
  root_ = normalize_node(std::move(root_));
}

void RopeTextBufferCore::erase_lines(size_t start_row, size_t end_row) {
  size_t L = count_lines(root_.get());
  if (end_row < start_row) end_row = start_row;
  if (start_row >= L) return; if (end_row > L) end_row = L;
  auto [A, B] = split(std::move(root_), start_row);
  auto [M, C] = split(std::move(B), end_row - start_row);
  root_ = concat(std::move(A), std::move(C));
  root_ = normalize_node(std::move(root_));
}

void RopeTextBufferCore::replace_line(size_t row, const std::string& s) { replace_line(row, std::string_view(s)); }

void RopeTextBufferCore::replace_line(size_t row, std::string_view s) {
  size_t L = count_lines(root_.get()); if (row >= L) return;
  auto [A, B] = split(std::move(root_), row);
  auto [M, C] = split(std::move(B), 1);
  std::vector<std::string> single{std::string(s)};
  auto M2 = make_leaf(std::move(single));
  root_ = concat(concat(std::move(A), std::move(M2)), std::move(C));
  root_ = normalize_node(std::move(root_));
}
