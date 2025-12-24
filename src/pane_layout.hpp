#pragma once
#include <memory>
#include <vector>

struct Rect {
  int row = 0;
  int col = 0;
  int height = 0;
  int width = 0;
};

struct PaneRect {
  int pane = 0;
  Rect rect;
};

struct SplitNode {
  enum class Type { Leaf, Vertical, Horizontal };
  Type type = Type::Leaf;
  int pane = 0; // valid when leaf
  std::unique_ptr<SplitNode> a;
  std::unique_ptr<SplitNode> b;
  float ratio = 0.5f; // left/top share for split nodes
};

void collect_layout(const SplitNode& node, const Rect& area, std::vector<PaneRect>& out);
bool replace_leaf_with_vertical(std::unique_ptr<SplitNode>& root, int target_pane, int new_pane, float ratio);
bool replace_leaf_with_horizontal(std::unique_ptr<SplitNode>& root, int target_pane, int new_pane, float ratio);
bool remove_leaf(std::unique_ptr<SplitNode>& root, int target_pane);
