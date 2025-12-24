#include "pane_layout.hpp"
#include <cassert>
#include <vector>

static int width_for_pane(const std::vector<PaneRect>& rs, int pane) {
  for (const auto& r : rs) if (r.pane == pane) return r.rect.width;
  return -1;
}

static int height_for_pane(const std::vector<PaneRect>& rs, int pane) {
  for (const auto& r : rs) if (r.pane == pane) return r.rect.height;
  return -1;
}

void run_layout_tests() {
  auto root = std::make_unique<SplitNode>();
  root->type = SplitNode::Type::Leaf;
  root->pane = 0;
  Rect screen{0, 0, 24, 80};
  std::vector<PaneRect> rs;
  collect_layout(*root, screen, rs);
  assert(rs.size() == 1);
  assert(rs[0].rect.width == 80);

  replace_leaf_with_vertical(root, 0, 1, 0.5f);
  rs.clear();
  collect_layout(*root, screen, rs);
  assert(rs.size() == 2);
  assert(width_for_pane(rs, 0) + width_for_pane(rs, 1) == 80);

  replace_leaf_with_horizontal(root, 1, 2, 0.5f);
  rs.clear();
  collect_layout(*root, screen, rs);
  assert(rs.size() == 3);
  assert(height_for_pane(rs, 2) > 0);

  remove_leaf(root, 2);
  rs.clear();
  collect_layout(*root, screen, rs);
  assert(rs.size() == 2);
}
