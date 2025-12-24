#include "pane_layout.hpp"
#include <algorithm>

static int clamp_split(int total, float ratio) {
  if (total <= 1) return total;
  int primary = static_cast<int>(total * ratio);
  primary = std::clamp(primary, 1, total - 1);
  return primary;
}

void collect_layout(const SplitNode& node, const Rect& area, std::vector<PaneRect>& out) {
  if (area.height <= 0 || area.width <= 0) return;
  if (node.type == SplitNode::Type::Leaf) {
    out.push_back(PaneRect{node.pane, area});
    return;
  }
  if (node.type == SplitNode::Type::Vertical) {
    int left_w = clamp_split(area.width, node.ratio);
    Rect left{area.row, area.col, area.height, left_w};
    Rect right{area.row, area.col + left_w, area.height, area.width - left_w};
    if (node.a) collect_layout(*node.a, left, out);
    if (node.b) collect_layout(*node.b, right, out);
  } else {
    int top_h = clamp_split(area.height, node.ratio);
    Rect top{area.row, area.col, top_h, area.width};
    Rect bottom{area.row + top_h, area.col, area.height - top_h, area.width};
    if (node.a) collect_layout(*node.a, top, out);
    if (node.b) collect_layout(*node.b, bottom, out);
  }
}

static bool replace_leaf(std::unique_ptr<SplitNode>& node, int target, int new_pane, SplitNode::Type t, float ratio) {
  if (!node) return false;
  if (node->type == SplitNode::Type::Leaf) {
    if (node->pane != target) return false;
    auto old = std::move(node);
    node = std::make_unique<SplitNode>();
    node->type = t;
    node->ratio = ratio;
    node->a = std::move(old);
    node->b = std::make_unique<SplitNode>();
    node->b->type = SplitNode::Type::Leaf;
    node->b->pane = new_pane;
    return true;
  }
  return replace_leaf(node->a, target, new_pane, t, ratio) || replace_leaf(node->b, target, new_pane, t, ratio);
}

bool replace_leaf_with_vertical(std::unique_ptr<SplitNode>& root, int target_pane, int new_pane, float ratio) {
  return replace_leaf(root, target_pane, new_pane, SplitNode::Type::Vertical, ratio);
}

bool replace_leaf_with_horizontal(std::unique_ptr<SplitNode>& root, int target_pane, int new_pane, float ratio) {
  return replace_leaf(root, target_pane, new_pane, SplitNode::Type::Horizontal, ratio);
}

static bool remove_leaf_internal(std::unique_ptr<SplitNode>& node, int target) {
  if (!node) return false;
  if (node->type == SplitNode::Type::Leaf) {
    if (node->pane != target) return false;
    node.reset();
    return true;
  }
  bool removed_left = remove_leaf_internal(node->a, target);
  bool removed_right = false;
  if (!removed_left) removed_right = remove_leaf_internal(node->b, target);
  if (node->a && !node->b) node = std::move(node->a);
  else if (!node->a && node->b) node = std::move(node->b);
  return removed_left || removed_right;
}

bool remove_leaf(std::unique_ptr<SplitNode>& root, int target_pane) {
  if (!root) return false;
  if (root->type == SplitNode::Type::Leaf) return false; // single pane cannot be removed here
  bool removed = remove_leaf_internal(root, target_pane);
  return removed && root != nullptr;
}
