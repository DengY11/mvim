#include <ncurses.h>
#include "editor.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <functional>
#include <limits>
#include "file_reader.hpp"
#include "undo_manager.hpp"
#include "pane_layout.hpp"

static constexpr int CTRL_u = 'U'-64; 
static constexpr int CTRL_R = 'R'-64; 
static constexpr int CTRL_d = 'D'-64; 
static constexpr int CTRL_w = 'W'-64;
static constexpr int ESC = 27;
static inline bool is_space(unsigned char c) {
  return std::isspace(c) != 0;
}
static inline bool is_num(unsigned char c) {
  return std::isdigit(c) != 0;
}
static inline bool is_word(unsigned char c) {
  return std::isalnum(c) != 0 || c == '_';
}
static inline bool is_symbol(unsigned char c) {
  return !is_space(c) && !is_word(c);
}
static int next_word_start_same_line(const std::string& line, int col) {
  int len = static_cast<int>(line.size());
  if (col < 0) col = 0;
  if (col > len) col = len;
  if (col >= len) return len;
  unsigned char c = static_cast<unsigned char>(line[col]);
  if (is_space(c)) {
    while (col < len && is_space(static_cast<unsigned char>(line[col]))) col++;
    return col;
  }
  if (is_word(c) || is_num(c)) {
    while (col + 1 < len && (is_word(static_cast<unsigned char>(line[col + 1])) || is_num(static_cast<unsigned char>(line[col + 1])))) col++;
    return std::min(len, col + 1);
  }
  while (col + 1 < len && is_symbol(static_cast<unsigned char>(line[col + 1]))) col++;
  return std::min(len, col + 1);
}
static int next_word_end_same_line(const std::string& line, int col) {
  int len = static_cast<int>(line.size());
  if (col < 0) col = 0;
  if (col > len) col = len;
  if (col >= len) return len;
  unsigned char c = static_cast<unsigned char>(line[col]);
  if (is_space(c)) {
    while (col < len && is_space(static_cast<unsigned char>(line[col]))) col++;
    if (col >= len) return len;
    unsigned char c2 = static_cast<unsigned char>(line[col]);
    if (is_word(c2) || is_num(c2)) {
      while (col + 1 < len && (is_word(static_cast<unsigned char>(line[col + 1])) || is_num(static_cast<unsigned char>(line[col + 1])))) col++;
      return std::min(len, col + 1);
    }
    while (col + 1 < len && is_symbol(static_cast<unsigned char>(line[col + 1]))) col++;
    return std::min(len, col + 1);
  }
  if (is_word(c) || is_num(c)) {
    while (col + 1 < len && (is_word(static_cast<unsigned char>(line[col + 1])) || is_num(static_cast<unsigned char>(line[col + 1])))) col++;
    return std::min(len, col + 1);
  }
  while (col + 1 < len && is_symbol(static_cast<unsigned char>(line[col + 1]))) col++;
  return std::min(len, col + 1);
}
Editor::Pane& Editor::pane() { return panes[active_pane]; }
const Editor::Pane& Editor::pane() const { return panes[active_pane]; }
Editor::Document& Editor::doc() { return *pane().doc; }
const Editor::Document& Editor::doc() const { return *pane().doc; }

void Editor::set_active_pane(int idx) {
  if (idx < 0 || idx >= (int)panes.size()) return;
  if (idx == active_pane) return;
  commit_insert_buffer();
  active_pane = idx;
  input.reset();
  pending_op = PendingOp::None;
  visual_active = false;
  last_search_hits.clear();
}

static std::string normalize_key(const std::filesystem::path& p) {
  std::error_code ec;
  auto abs = std::filesystem::absolute(p, ec);
  auto norm = abs.lexically_normal();
  return norm.string();
}

int Editor::create_pane_from_file(const std::optional<std::filesystem::path>& file) {
  Pane p;
  if (file) {
    std::string key = normalize_key(*file);
    if (auto it = doc_table.find(key); it != doc_table.end()) {
      if (auto existing = it->second.lock()) {
        p.doc = existing;
      }
    }
    if (!p.doc) {
      auto d = std::make_shared<Document>();
      bool ok = true; std::string m;
      d->buf = TextBuffer::from_file(*file, m, ok);
      d->file_path = *file;
      message = m;
      d->modified = false;
      doc_table[key] = d;
      p.doc = d;
    }
  } else {
    auto d = std::make_shared<Document>();
    d->buf.ensure_not_empty();
    p.doc = d;
  }
  panes.push_back(std::move(p));
  return static_cast<int>(panes.size() - 1);
}

void Editor::split_vertical(const std::optional<std::filesystem::path>& file) {
  int new_idx = file ? create_pane_from_file(file) : create_pane_from_file(doc().file_path);
  if (!layout) {
    layout = std::make_unique<SplitNode>();
    layout->type = SplitNode::Type::Leaf;
    layout->pane = active_pane;
  }
  if (!replace_leaf_with_vertical(layout, active_pane, new_idx, 0.5f)) return;
  set_active_pane(new_idx);
}

void Editor::split_horizontal(const std::optional<std::filesystem::path>& file) {
  int new_idx = file ? create_pane_from_file(file) : create_pane_from_file(doc().file_path);
  if (!layout) {
    layout = std::make_unique<SplitNode>();
    layout->type = SplitNode::Type::Leaf;
    layout->pane = active_pane;
  }
  if (!replace_leaf_with_horizontal(layout, active_pane, new_idx, 0.5f)) return;
  set_active_pane(new_idx);
}

bool Editor::close_active_pane() {
  if (!layout) return false;
  if (layout->type == SplitNode::Type::Leaf) return false;
  int closing = active_pane;
  if (!remove_leaf(layout, closing)) return false;
  // pick first leaf as new active
  std::vector<PaneRect> rects;
  TermSize sz = term.getSize();
  Rect root_rect{0, 0, sz.rows, sz.cols};
  ::collect_layout(*layout, root_rect, rects);
  if (!rects.empty()) {
    set_active_pane(rects.front().pane);
  }
  return true;
}

bool Editor::close_or_quit(bool force) {
  int pane_cnt = active_pane_count();
  if (pane_cnt > 1) {
    if (!force && doc().modified) { message = "have unsaved changes, use :q! or :w"; return false; }
    if (!close_active_pane()) { message = "cannot close pane"; return false; }
    return true;
  }
  if (!force && doc().modified) { message = "have unsaved changes, use :q! or :w"; return false; }
  should_quit = true;
  return true;
}

void Editor::collect_layout(std::vector<PaneRect>& out) const {
  out.clear();
  TermSize sz = term.getSize();
  Rect root{0, 0, sz.rows, sz.cols};
  if (layout) ::collect_layout(*layout, root, out);
}

int Editor::active_pane_count() const {
  std::vector<PaneRect> rects;
  collect_layout(rects);
  return rects.empty() ? 1 : static_cast<int>(rects.size());
}

void Editor::focus_next_pane() {
  std::vector<PaneRect> rects;
  collect_layout(rects);
  if (rects.size() <= 1) return;
  // rects order is traversal order; find current index
  size_t idx = 0;
  for (size_t i = 0; i < rects.size(); ++i) if (rects[i].pane == active_pane) { idx = i; break; }
  size_t next = (idx + 1) % rects.size();
  set_active_pane(rects[next].pane);
}

void Editor::focus_direction(char dir) {
  std::vector<PaneRect> rects;
  collect_layout(rects);
  if (rects.size() <= 1) return;
  Rect cur_rect{};
  bool found = false;
  for (const auto& pr : rects) if (pr.pane == active_pane) { cur_rect = pr.rect; found = true; break; }
  if (!found) return;
  auto center = [](const Rect& r){ return std::pair<int,int>{r.row + r.height/2, r.col + r.width/2}; };
  auto [cr, cc] = center(cur_rect);
  int best_idx = -1;
  int best_score = std::numeric_limits<int>::max();
  for (const auto& pr : rects) {
    if (pr.pane == active_pane) continue;
    auto [rr, rc] = center(pr.rect);
    int dr = rr - cr;
    int dc = rc - cc;
    bool ok = false;
    switch (dir) {
      case 'h': ok = (dc < 0); break;
      case 'l': ok = (dc > 0); break;
      case 'k': ok = (dr < 0); break;
      case 'j': ok = (dr > 0); break;
      default: break;
    }
    if (!ok) continue;
    int score = dr*dr + dc*dc;
    if (score < best_score) { best_score = score; best_idx = pr.pane; }
  }
  if (best_idx >= 0) set_active_pane(best_idx);
  else focus_next_pane();
}

void Editor::open_path_in_pane(int idx, const std::filesystem::path& path) {
  if (idx < 0 || idx >= (int)panes.size()) return;
  Pane& p = panes[idx];
  std::string key = normalize_key(path);
  if (auto it = doc_table.find(key); it != doc_table.end()) {
    if (auto existing = it->second.lock()) p.doc = existing;
  }
  if (!p.doc || (p.doc->file_path && normalize_key(*p.doc->file_path) != key)) {
    auto d = std::make_shared<Document>();
    bool ok = true; std::string m;
    d->buf = TextBuffer::from_file(path, m, ok);
    d->modified = false;
    d->file_path = path;
    d->last_change.reset();
    d->um = UndoManager();
    doc_table[key] = d;
    p.doc = d;
    if (idx == active_pane) message = m;
  }
}


Editor::Editor(const std::optional<std::filesystem::path>& file) {
  active_pane = create_pane_from_file(file);
  layout = std::make_unique<SplitNode>();
  layout->type = SplitNode::Type::Leaf;
  layout->pane = active_pane;
  register_commands();
  load_rc();
}

void Editor::run() {
  while (!should_quit) {
    render();
    int ch = getch();
    handle_input(ch);
  }
}

void Editor::begin_group() { doc().um.begin_group(pane().cur); }
void Editor::commit_group() {
  size_t before = doc().um.undo_size();
  doc().um.commit_group(pane().cur);
  size_t after = doc().um.undo_size();
  if (after > before) {
    if (const UndoEntry* e = doc().um.last_entry()) doc().last_change = *e;
  }
}
void Editor::push_op(const Operation& op) { doc().um.push_op(op); }

void Editor::render() {
  int override_row = insert_buffer_active ? insert_buffer_row : -1;
  std::string override_line = insert_buffer_active ? insert_buffer_line : std::string();
  std::vector<PaneRect> rects;
  collect_layout(rects);
  if (rects.empty() && layout) {
    TermSize sz = term.getSize();
    rects.push_back({active_pane, Rect{0,0,sz.rows, sz.cols}});
  }
  std::vector<PaneRenderInfo> infos;
  infos.reserve(rects.size());
  for (const auto& pr : rects) {
    if (pr.pane < 0 || pr.pane >= (int)panes.size()) continue;
    const Pane& p = panes[pr.pane];
    PaneRenderInfo info;
    info.buf = &p.doc->buf;
    info.cur = p.cur;
    info.vp = const_cast<Viewport*>(&p.vp);
    info.file_path = p.doc->file_path;
    info.modified = p.doc->modified;
    info.is_active = (pr.pane == active_pane);
    info.area = pr.rect;
    if (info.is_active) { info.override_row = override_row; info.override_line = override_line; }
    infos.push_back(std::move(info));
  }
  renderer.render(term, infos, mode, message, cmdline, visual_active, visual_anchor, show_line_numbers, relative_line_numbers, enable_color, last_search_hits);
}

void Editor::handle_input(int ch) {
  if (enable_mouse && ch == KEY_MOUSE) { handle_mouse(); return; }
  if (mode == Mode::Command) { handle_command_input(ch); return; }
  if (mode == Mode::Insert) { handle_insert_input(ch); return; }
  handle_normal_input(ch);
}

void Editor::handle_normal_input(int ch) {
  if (pending_ctrl_w) {
    pending_ctrl_w = false;
    switch (ch) {
      case 'h': case 'j': case 'k': case 'l': focus_direction((char)ch); return;
      case 'w': focus_next_pane(); return;
      default: break;
    }
  }
  if (ch != 'd' && ch != 'y' && ch != 'g' && ch != '>' && ch != '<') input.reset();
  switch (ch) {
    case CTRL_w: pending_ctrl_w = true; break;
    case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
      if (input.consumeDigit(ch)) break;
      break;
    case '0':
      if (input.hasCount()) { if (input.consumeDigit('0')) break; }
      pane().cur.col = 0; break;

    case 'h': {
      size_t n = input.takeCount(); if (n == 0) n = 1; while (n--) move_left();
    } break;
    case 'j': {
      size_t n = input.takeCount(); if (n == 0) n = 1; while (n--) move_down();
    } break;
    case 'k': {
      size_t n = input.takeCount(); if (n == 0) n = 1; while (n--) move_up();
    } break;
    case 'l': {
      size_t n = input.takeCount(); if (n == 0) n = 1; while (n--) move_right();
    } break;
    case 'x': begin_group(); delete_char(); commit_group(); break;
    case 'i': begin_group(); mode = Mode::Insert; break;
    case 'a': begin_group(); pane().cur.col = std::min((int)doc().buf.line(pane().cur.row).size(), pane().cur.col + 1); mode = Mode::Insert; break;
    case 'o': {
      std::string indent; if (auto_indent) indent = compute_indent_for_line(pane().cur.row);
      begin_group();
      insert_line_below();
      if (auto_indent && !indent.empty()) apply_indent_to_newline(indent);
      mode = Mode::Insert;
    } break;
    case 'O': {
      std::string indent; if (auto_indent) indent = compute_indent_for_line(pane().cur.row);
      begin_group();
      insert_line_above();
      if (auto_indent && !indent.empty()) apply_indent_to_newline(indent);
      mode = Mode::Insert;
    } break;
    case '.':
      repeat_last_change();
      pending_op = PendingOp::None;
      input.reset();
      break;
    case ':': mode = Mode::Command; cmdline.clear(); break;
    case '/': mode = Mode::Command; cmdline = "/"; break;
    case '?': mode = Mode::Command; cmdline = "?"; break;
    case ESC: // ESC
      if (mode == Mode::Visual || mode == Mode::VisualLine) { exit_visual(); break; }
      break;
    case 'v':
      if (mode == Mode::Visual || mode == Mode::VisualLine) { exit_visual(); }
      else { enter_visual_char(); }
      break;
    case 'V':
      if (mode == Mode::Visual || mode == Mode::VisualLine) { exit_visual(); }
      else { enter_visual_line(); }
      break;
    case 'u': undo(); break;
    case CTRL_R: redo(); break;
    case 'd':
      if (mode == Mode::Visual || mode == Mode::VisualLine) { delete_selection(); exit_visual(); }
      else if (input.consumeDd('d')) { size_t n = input.takeCount(); if (n == 0) n = 1; begin_group(); delete_lines_range(pane().cur.row, (int)n); commit_group();input.reset();pending_op = PendingOp::None; }
      else { pending_op = PendingOp::Delete; }
      break;
    case 'y':
      if (mode == Mode::Visual || mode == Mode::VisualLine) { yank_selection(); exit_visual(); }
      else if (input.consumeYy('y')) {
        size_t n = input.takeCount(); if (n == 0) n = 1;
        reg.lines.clear(); reg.linewise = true; reg.lines.reserve(n);
        for (int i = 0; i < (int)n; ++i) {
          int r = pane().cur.row + i; if (r < 0 || r >= doc().buf.line_count()) break;
          reg.lines.push_back(doc().buf.line(r));
        }
        input.reset(); pending_op = PendingOp::None;
      }
      else { pending_op = PendingOp::Yank; }
      break;
    case 'p': paste_below(); break;
    case 'g': if (input.consumeGg('g')) {
      size_t n = input.takeCount();
      if (n == 0) { move_to_top(); }
      else {
        int target = static_cast<int>(std::max<size_t>(1, n)) - 1;
        pane().cur.row = std::min(target, std::max(0, doc().buf.line_count() - 1));
        pane().cur.col = std::min(pane().cur.col, (int)doc().buf.line(pane().cur.row).size());
      }
    } break;
    case 'G': {
      size_t n = input.takeCount();
      if (n == 0) { move_to_bottom(); }
      else {
        int target = static_cast<int>(std::max<size_t>(1, n)) - 1;
        pane().cur.row = std::min(target, std::max(0, doc().buf.line_count() - 1));
        pane().cur.col = std::min(pane().cur.col, (int)doc().buf.line(pane().cur.row).size());
      }
    } break;
    case 'w': {
      size_t n = input.takeCount(); if (n == 0) n = 1;
      if (pending_op == PendingOp::Delete) {
        // support count: repeat n times
        while (n--) delete_to_next_word();
        pending_op = PendingOp::None;
      } else if (pending_op == PendingOp::Yank) {
        while (n--) yank_to_next_word();
        pending_op = PendingOp::None;
      } else {
        while (n--) move_to_next_word_left();
      }
    } break;
    case 'e': {
      size_t n = input.takeCount(); if (n == 0) n = 1;
      if (pending_op == PendingOp::Delete) {
        while (n--) delete_to_word_end();
        pending_op = PendingOp::None;
      } else if (pending_op == PendingOp::Yank) {
        while (n--) yank_to_word_end();
        pending_op = PendingOp::None;
      } else {
        while (n--) move_to_next_word_right();
      }
    } break;
    case 'b': { size_t n = input.takeCount(); if (n == 0) n = 1; while (n--) move_to_previous_word_left(); } break;
    case '^': move_to_beginning_of_line(); break;
    case '$': move_to_end_of_line(); break;
    case CTRL_d: scroll_down_half_page(); break;
    case CTRL_u: scroll_up_half_page(); break;
    case 'n': {
      size_t k = input.takeCount(); if (k == 0) k = 1; while (k--) repeat_last_search(last_search_forward);
    } break;
    case 'N': {
      size_t k = input.takeCount(); if (k == 0) k = 1; while (k--) repeat_last_search(!last_search_forward);
    } break;
    case '>': {
      if (mode == Mode::Visual || mode == Mode::VisualLine) {
        int r0, r1, c0, c1; get_visual_range(r0, r1, c0, c1);
        begin_group();
        indent_lines(r0, r1 - r0 + 1);
        commit_group();
        exit_visual();
        input.reset();
        break;
      }
      if (input.consumeGt('>')) {
        size_t n = input.takeCount(); if (n == 0) n = 1;
        begin_group();
        indent_lines(pane().cur.row, (int)n);
        commit_group();
        input.reset();
      }
    } break;
    case '<': {
      if (mode == Mode::Visual || mode == Mode::VisualLine) {
        int r0, r1, c0, c1; get_visual_range(r0, r1, c0, c1);
        begin_group();
        dedent_lines(r0, r1 - r0 + 1);
        commit_group();
        exit_visual();
        input.reset();
        break;
      }
      if (input.consumeLt('<')) {
        size_t n = input.takeCount(); if (n == 0) n = 1;
        begin_group();
        dedent_lines(pane().cur.row, (int)n);
        commit_group();
        input.reset();
      }
    } break;
    default: input.reset(); break;
  }
}

void Editor::handle_insert_input(int ch) {
  if (ch == ESC) { commit_insert_buffer(); commit_group(); mode = Mode::Normal; return; }
  if (ch == KEY_BACKSPACE || ch == 127) { apply_backspace(); return; }
  if (ch == '\t') {
    if (!insert_buffer_active || insert_buffer_row != pane().cur.row) begin_insert_buffer();
    int n = std::max(1, tab_width);
    for (int i = 0; i < n; ++i) { apply_insert_char(' '); }
    return;
  }
  if (ch == '('|| ch == '{'||ch == '[') {
    if (auto_pair) {
      if (!insert_buffer_active || insert_buffer_row != pane().cur.row) begin_insert_buffer();
      char opening = (char)ch;
      char closing = (opening=='(')?')':(opening=='{'?'}':']');
      apply_insert_char(opening);
      if (!(pane().cur.col < (int)insert_buffer_line.size() && insert_buffer_line[pane().cur.col] == closing)) {
        apply_insert_char(closing);
        pane().cur.col = std::max(0, pane().cur.col - 1);
      }
      return;
    }
  }
  if (ch == '\n' || ch == KEY_ENTER || ch == '\r') {
    std::string indent;
    if (auto_indent) {
      if (insert_buffer_active && insert_buffer_row == pane().cur.row) indent = compute_indent_for_line(pane().cur.row);
      else indent = compute_indent_for_line(pane().cur.row);
    }
    begin_group();
    commit_insert_buffer();
    split_line_at_cursor();
    if (auto_indent) apply_indent_to_newline(indent);
    commit_group();
    begin_insert_buffer();
    return;
  }
  if (ch >= 32 && ch <= 126) {
    apply_insert_char(ch);
    return;
  }
}

void Editor::begin_insert_buffer() {
  insert_buffer_active = true;
  insert_buffer_row = pane().cur.row;
  insert_buffer_line = doc().buf.line(pane().cur.row);
}

void Editor::apply_insert_char(int ch) {
  if (!insert_buffer_active || insert_buffer_row != pane().cur.row) begin_insert_buffer();
  begin_group();
  if (pane().cur.col < 0) pane().cur.col = 0;
  if (pane().cur.col > static_cast<int>(insert_buffer_line.size())) pane().cur.col = static_cast<int>(insert_buffer_line.size());
  insert_buffer_line.insert(insert_buffer_line.begin() + pane().cur.col, static_cast<char>(ch));
  push_op({Operation::InsertChar, pane().cur.row, pane().cur.col, std::string(1, (char)ch), std::string()});
  doc().modified = true;
  pane().cur.col++;
  doc().buf.replace_line(insert_buffer_row, insert_buffer_line);
  doc().um.clear_redo();
  commit_group();
}

void Editor::apply_backspace() {
  if (!insert_buffer_active || insert_buffer_row != pane().cur.row) begin_insert_buffer();
  if (pane().cur.col > 0) {
    begin_group();
    char c = insert_buffer_line[pane().cur.col - 1];
    insert_buffer_line.erase(insert_buffer_line.begin() + pane().cur.col - 1);
    push_op({Operation::DeleteChar, pane().cur.row, pane().cur.col - 1, std::string(1, c), std::string()});
    pane().cur.col--; doc().modified = true;
    doc().buf.replace_line(insert_buffer_row, insert_buffer_line);
    doc().um.clear_redo();
    commit_group();
  } else {
    // At line start: commit buffer then use existing merge logic
    commit_insert_buffer();
    backspace();
    begin_insert_buffer();
  }
}

void Editor::commit_insert_buffer() {
  if (insert_buffer_active) {
    std::string old = doc().buf.line(insert_buffer_row);
    std::string neu = insert_buffer_line;
    // 若已在逐字符写回，则 old == neu，不再推送整行 ReplaceLine，以避免一次性撤销整行
    if (old != neu) {
      doc().buf.replace_line(insert_buffer_row, neu);
      push_op({Operation::ReplaceLine, insert_buffer_row, pane().cur.col, old, neu});
      doc().um.clear_redo();
    }
    insert_buffer_active = false;
    insert_buffer_row = -1;
    insert_buffer_line.clear();
  }
}

void Editor::insert_pair(int ch) {
  char closing = 0;
  switch (ch) {
    case '(': closing = ')'; break;
    case '{': closing = '}'; break;
    case '[': closing = ']'; break;
    default: return;
  }
  std::string s = doc().buf.line(pane().cur.row);
  if (pane().cur.col < 0) pane().cur.col = 0;
  if (pane().cur.col > (int)s.size()) pane().cur.col = (int)s.size();
  s.insert(s.begin() + pane().cur.col, (char)ch);
  push_op({Operation::InsertChar, pane().cur.row, pane().cur.col, std::string(1, (char)ch), std::string()});
  doc().modified = true;
  pane().cur.col++;
  if (!(pane().cur.col < (int)s.size() && s[pane().cur.col] == closing)) {
    s.insert(s.begin() + pane().cur.col, closing);
    push_op({Operation::InsertChar, pane().cur.row, pane().cur.col, std::string(1, closing), std::string()});
    doc().modified = true;
  }
  doc().buf.replace_line(pane().cur.row, s);
  doc().um.clear_redo();
}

void Editor::handle_command_input(int ch) {
  if (ch == ESC) { mode = Mode::Normal; return; }
  if (ch == KEY_BACKSPACE || ch == 127) { if (!cmdline.empty()) cmdline.pop_back(); return; }
  if (ch == '\n' || ch == KEY_ENTER || ch == '\r') { execute_command(); mode = Mode::Normal; return; }
  if (ch >= 32 && ch <= 126) { cmdline.push_back((char)ch); }
}

void Editor::handle_mouse() {
  MEVENT me; if (getmouse(&me) != OK) return;
  std::vector<PaneRect> rects; collect_layout(rects);
  Rect area{0,0,term.getSize().rows, term.getSize().cols};
  for (const auto& pr : rects) {
    const Rect& r = pr.rect;
    if (me.y >= r.row && me.y < r.row + r.height && me.x >= r.col && me.x < r.col + r.width) {
      set_active_pane(pr.pane);
      area = r;
      break;
    }
  }
  int inner_rows = std::max(0, area.height - 2);
  int inner_cols = std::max(0, area.width - 2);
  if (inner_rows <= 0 || inner_cols <= 0) return;
  int max_text_rows = std::max(0, inner_rows - 1);
  int wheel_step = 3;
  if (max_text_rows > 0) wheel_step = std::max(1, max_text_rows / 6);
  #ifdef BUTTON4_PRESSED
  if (me.bstate & BUTTON4_PRESSED) {
    int max_top = std::max(0, doc().buf.line_count() - max_text_rows);
    pane().vp.top_line = std::max(0, pane().vp.top_line - wheel_step);
    pane().vp.top_line = std::min(pane().vp.top_line, max_top);
    return;
  }
  #endif
  #ifdef BUTTON5_PRESSED
  if (me.bstate & BUTTON5_PRESSED) {
    int max_top = std::max(0, doc().buf.line_count() - max_text_rows);
    pane().vp.top_line = std::min(max_top, pane().vp.top_line + wheel_step);
    return;
  }
  #endif
  int inner_row_off = area.row + 1;
  int inner_col_off = area.col + 1;
  if (me.y < inner_row_off || me.y >= inner_row_off + inner_rows) return;
  if (me.y == inner_row_off + inner_rows - 1) return; // ignore status/command line
  int digits = 1; int total = std::max(1, doc().buf.line_count());
  while (total >= 10) { total /= 10; digits++; }
  int indent = show_line_numbers ? (digits + 1) : 0;
  int screen_row = me.y - inner_row_off;
  int buf_row = pane().vp.top_line + screen_row;
  buf_row = std::clamp(buf_row, 0, std::max(0, doc().buf.line_count() - 1));
  int screen_col = me.x - inner_col_off;
  if (screen_col < indent) return; // clicking in line-number gutter does nothing
  int rel = screen_col - indent;
  int buf_col = pane().vp.left_col + rel;
  int line_len = (buf_row >= 0 && buf_row < doc().buf.line_count()) ? (int)doc().buf.line(buf_row).size() : 0;
  int maxc = virtualedit_onemore ? line_len : (line_len > 0 ? line_len - 1 : 0);
  buf_col = std::clamp(buf_col, 0, maxc);
  if (me.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED)) {
    pane().cur.row = buf_row; pane().cur.col = buf_col;
  }
}

void Editor::execute_command() {
  if (!cmdline.empty() && (cmdline[0] == '/' || cmdline[0] == '?')) {
    std::string pat = cmdline.substr(1);
    last_search = pat;
    last_search_forward = (cmdline[0] == '/');
    if (last_search_forward) search_forward(pat); else search_backward(pat);
    recompute_search_hits(pat);
    return;
  }
  std::istringstream iss(cmdline);
  std::string cmd; iss >> cmd;
  std::vector<std::string> args; std::string a; while (iss >> a) args.push_back(a);
  if (cmd == "set" && !args.empty()) {
    std::string opt = args[0];
    std::string name = opt;
    std::string value;
    size_t eq = opt.find('=');
    if (eq != std::string::npos) {
      name = opt.substr(0, eq);
      value = opt.substr(eq + 1);
    }
    std::string composite = std::string("set ") + name;
    std::vector<std::string> subargs;
    if (!value.empty()) subargs.push_back(value);
    for (size_t i = 1; i < args.size(); ++i) subargs.push_back(args[i]);
    if (!registry.execute(composite, subargs)) { message = "unknown command: " + composite; }
    return;
  }
  if (!registry.execute(cmd, args)) { message = "unknown command: " + cmd; }
}

void Editor::delete_to_next_word() {
  Cursor old = pane().cur;
  const auto& s = doc().buf.line(old.row);
  int end_col = next_word_start_same_line(s, old.col);
  Cursor end{old.row, end_col};
  if (end.col <= old.col) return;
  begin_group();
  reg.lines.clear(); reg.linewise = false;
  int c0 = std::clamp(old.col, 0, (int)s.size());
  int c1 = std::clamp(end.col, 0, (int)s.size());
  reg.lines = { s.substr(c0, c1 - c0) };
  std::string neu = s.substr(0, c0) + s.substr(c1);
  push_op({Operation::ReplaceLine, old.row, c0, s, neu});
  doc().buf.replace_line(old.row, neu);
  pane().cur.row = old.row; pane().cur.col = c0;
  doc().modified = true; doc().um.clear_redo();
  commit_group();
}

void Editor::yank_to_next_word() {
  Cursor old = pane().cur;
  const auto& s = doc().buf.line(old.row);
  int end_col = next_word_start_same_line(s, old.col);
  if (end_col <= old.col) return;
  int c0 = std::clamp(old.col, 0, (int)s.size());
  int c1 = std::clamp(end_col, 0, (int)s.size());
  reg.lines.clear(); reg.linewise = false;
  reg.lines = { s.substr(c0, c1 - c0) };
}

void Editor::delete_to_word_end() {
  Cursor old = pane().cur;
  const auto& s = doc().buf.line(old.row);
  int end_col = next_word_end_same_line(s, old.col);
  Cursor end{old.row, end_col};
  if (end.col <= old.col) return;
  begin_group();
  reg.lines.clear(); reg.linewise = false;
  int c0 = std::clamp(old.col, 0, (int)s.size());
  int c1 = std::clamp(end.col, 0, (int)s.size());
  reg.lines = { s.substr(c0, c1 - c0) };
  std::string neu = s.substr(0, c0) + s.substr(c1);
  push_op({Operation::ReplaceLine, old.row, c0, s, neu});
  doc().buf.replace_line(old.row, neu);
  pane().cur.row = old.row; pane().cur.col = c0;
  doc().modified = true; doc().um.clear_redo();
  commit_group();
}

void Editor::yank_to_word_end() {
  Cursor old = pane().cur;
  const auto& s = doc().buf.line(old.row);
  int end_col = next_word_end_same_line(s, old.col);
  if (end_col <= old.col) return;
  int c0 = std::clamp(old.col, 0, (int)s.size());
  int c1 = std::clamp(end_col, 0, (int)s.size());
  reg.lines.clear(); reg.linewise = false;
  reg.lines = { s.substr(c0, c1 - c0) };
}

void Editor::load_rc() {
  std::error_code ec;
  const char* home = std::getenv("HOME");
  if (!home) return;
  auto p = std::filesystem::path(home) / ".mvimrc";
  if (!std::filesystem::exists(p, ec)) return;
  std::vector<std::string> lines; std::string msg;
  if (!mmap_readlines(p, lines, msg)) { message = msg; return; }
  for (std::string s : lines) {
    auto isspace_fn = [](unsigned char c){ return std::isspace(c) != 0; };
    size_t i = 0; while (i < s.size() && isspace_fn((unsigned char)s[i])) i++;
    size_t j = s.size(); while (j > i && isspace_fn((unsigned char)s[j-1])) j--;
    s = (j > i) ? s.substr(i, j - i) : std::string();
    if (s.empty()) continue;
    if (s[0] == '#' || s[0] == '"') continue;
    if (s.size() >= 2 && s[0] == '/' && s[1] == '/') continue;
    if (!s.empty() && s[0] == ':') s.erase(s.begin());
    std::string old = cmdline;
    cmdline = s;
    execute_command();
    cmdline = old;
  }
}

static std::vector<std::string> split_block_lines(const std::string& block) {
  std::vector<std::string> lines;
  size_t st = 0;
  while (st <= block.size()) {
    size_t pos = block.find('\n', st);
    if (pos == std::string::npos) { lines.emplace_back(block.substr(st)); break; }
    lines.emplace_back(block.substr(st, pos - st));
    st = pos + 1;
  }
  return lines;
}

static std::vector<int> kmp_build(const std::string& pat) {
  std::vector<int> pi(pat.size(), 0);
  for (size_t i = 1, j = 0; i < pat.size(); ++i) {
    while (j > 0 && pat[i] != pat[j]) j = pi[j - 1];
    if (pat[i] == pat[j]) ++j;
    pi[i] = (int)j;
  }
  return pi;
}

static int kmp_find_first_from(const std::string& s, const std::string& pat, size_t start) {
  if (pat.empty()) return -1;
  auto pi = kmp_build(pat);
  size_t j = 0;
  for (size_t i = start; i < s.size(); ++i) {
    while (j > 0 && s[i] != pat[j]) j = pi[j - 1];
    if (s[i] == pat[j]) ++j;
    if (j == pat.size()) return (int)(i + 1 - pat.size());
  }
  return -1;
}

static void kmp_find_all(const std::string& s, const std::string& pat, std::vector<int>& out) {
  out.clear(); if (pat.empty()) return;
  auto pi = kmp_build(pat);
  size_t j = 0;
  for (size_t i = 0; i < s.size(); ++i) {
    while (j > 0 && s[i] != pat[j]) j = pi[j - 1];
    if (s[i] == pat[j]) ++j;
    if (j == pat.size()) { out.push_back((int)(i + 1 - pat.size())); j = pi[j - 1]; }
  }
}

void Editor::search_forward(const std::string& pattern) {
  if (pattern.empty()) { message = "pattern empty"; return; }
  int rows = doc().buf.line_count();
  {
    const auto& line = doc().buf.line(pane().cur.row);
    int pos = kmp_find_first_from(line, pattern, (size_t)std::min((int)line.size(), pane().cur.col + 1));
    if (pos >= 0) { pane().cur.col = pos; return; }
  }
  for (int r = pane().cur.row + 1; r < rows; ++r) {
    const auto& s = doc().buf.line(r);
    int pos = kmp_find_first_from(s, pattern, 0);
    if (pos >= 0) { pane().cur.row = r; pane().cur.col = pos; return; }
  }
  message = "not found pattern";
}

void Editor::search_backward(const std::string& pattern) {
  if (pattern.empty()) { message = "pattern empty"; return; }
  {
    const auto& line = doc().buf.line(pane().cur.row);
    std::vector<int> pos;
    kmp_find_all(line, pattern, pos);
    int target = -1; for (int p : pos) if (p < pane().cur.col) target = p; 
    if (target >= 0) { pane().cur.col = target; return; }
  }
  for (int r = pane().cur.row - 1; r >= 0; --r) {
    const auto& s = doc().buf.line(r);
    std::vector<int> pos;
    kmp_find_all(s, pattern, pos);
    if (!pos.empty()) { pane().cur.row = r; pane().cur.col = pos.back(); return; }
  }
  message = "not found pattern";
}

void Editor::repeat_last_search(bool is_forward) {
  if (last_search.empty()) { message = "no last search"; return; }
  if (is_forward) search_forward(last_search); else search_backward(last_search);
  recompute_search_hits(last_search);
}

void Editor::recompute_search_hits(const std::string& pattern) {
  last_search_hits.clear();
  if (pattern.empty()) return;
  int rows = doc().buf.line_count();
  for (int r = 0; r < rows; ++r) {
    const auto& s = doc().buf.line(r);
    std::vector<int> pos;
    kmp_find_all(s, pattern, pos);
    for (int p : pos) last_search_hits.push_back({r, p, (int)pattern.size()});
  }
  if (!last_search_hits.empty()) {
    const SearchHit* next = nullptr;
    for (const auto& h : last_search_hits) { if (h.row > pane().cur.row || (h.row == pane().cur.row && h.col >= pane().cur.col)) { next = &h; break; } }
    if (!next) next = &last_search_hits.front();
    message = std::string("matches:") + std::to_string((int)last_search_hits.size()) + " next " + std::to_string(next->row + 1) + ":" + std::to_string(next->col + 1);
  } else {
    message = "not found pattern";
  }
}

void Editor::enter_visual_char() {
  visual_active = true;
  visual_anchor = pane().cur;
  mode = Mode::Visual;
}

void Editor::enter_visual_line() {
  visual_active = true;
  visual_anchor = {pane().cur.row, 0};
  mode = Mode::VisualLine;
}

void Editor::exit_visual() {
  visual_active = false;
  mode = Mode::Normal;
}

void Editor::get_visual_range(int& r0, int& r1, int& c0, int& c1) const {
  r0 = std::min(visual_anchor.row, pane().cur.row);
  r1 = std::max(visual_anchor.row, pane().cur.row);
  if (mode == Mode::VisualLine) {
    c0 = 0; c1 = -1; 
  } else {
    int a = visual_anchor.col;
    int b = pane().cur.col;
    c0 = std::min(a, b);
    c1 = std::max(a, b) + 1; 
  }
}

void Editor::delete_selection() {
  int r0, r1, c0, c1; get_visual_range(r0, r1, c0, c1);
  begin_group();
  yank_selection();
  if (mode == Mode::VisualLine) {
    int end = std::min(r1 + 1, doc().buf.line_count());
    std::string block;
    for (int k = r0; k <= std::min(r1, doc().buf.line_count() - 1); ++k) {
      block += doc().buf.line(k);
      if (k < r1) block += '\n';
    }
    push_op({Operation::DeleteLinesBlock, r0, 0, block, std::string()});
    if (r0 < end) doc().buf.erase_lines(r0, end);
    pane().cur.row = std::min(r0, doc().buf.line_count() - 1);
    pane().cur.col = 0;
  } else {
    if (r0 == r1) {
      std::string s = doc().buf.line(r0);
      c0 = std::clamp(c0, 0, (int)s.size());
      c1 = std::clamp(c1, 0, (int)s.size());
      if (c1 > c0) {
        std::string old = s;
        std::string neu = old.substr(0, c0) + old.substr(c1);
        push_op({Operation::ReplaceLine, r0, c0, old, neu});
        doc().buf.replace_line(r0, neu);
      }
      pane().cur.row = r0; pane().cur.col = c0;
    } else {
      std::string first = doc().buf.line(r0);
      std::string last = doc().buf.line(r1);
      c0 = std::clamp(c0, 0, (int)first.size());
      c1 = std::clamp(c1, 0, (int)last.size());
      std::string old_first = first;
      std::string left = first.substr(0, c0);
      std::string right = last.substr(c1);
      std::string neu_first = left + right;
      push_op({Operation::ReplaceLine, r0, c0, old_first, neu_first});
      if (r1 > r0 + 1) {
        std::string mid;
        for (int k = r0 + 1; k <= r1; ++k) {
          mid += doc().buf.line(k);
          if (k < r1) mid += '\n';
        }
        push_op({Operation::DeleteLinesBlock, r0 + 1, 0, mid, std::string()});
      }
      doc().buf.replace_line(r0, neu_first);
      doc().buf.erase_lines(r0 + 1, r1 + 1);
      pane().cur.row = r0; pane().cur.col = (int)left.size();
    }
  }
  doc().modified = true; doc().um.clear_redo();
  commit_group();
}

void Editor::yank_selection() {
  int r0, r1, c0, c1; get_visual_range(r0, r1, c0, c1);
  reg.lines.clear();
  if (mode == Mode::VisualLine) {
    for (int r = r0; r <= r1; ++r) reg.lines.push_back(doc().buf.line(r));
    reg.linewise = true;
  } else {
    if (r0 == r1) {
      const auto& s = doc().buf.line(r0);
      c0 = std::clamp(c0, 0, (int)s.size());
      c1 = std::clamp(c1, 0, (int)s.size());
      if (c1 > c0) reg.lines = { s.substr(c0, c1 - c0) }; else reg.lines = { "" };
      reg.linewise = false;
    } else {
      const auto& first = doc().buf.line(r0);
      const auto& last = doc().buf.line(r1);
      c0 = std::clamp(c0, 0, (int)first.size());
      c1 = std::clamp(c1, 0, (int)last.size());
      std::string out;
      out += first.substr(c0);
      out += '\n';
      for (int r = r0 + 1; r <= r1 - 1; ++r) { out += doc().buf.line(r); out += '\n'; }
      out += last.substr(0, c1);
      reg.lines = { out };
      reg.linewise = false;
    }
  }
}

void Editor::move_left() { if (pane().cur.col > 0) pane().cur.col--; }
void Editor::move_right() {
  int maxc = max_col_for_row(pane().cur.row);
  pane().cur.col = std::min(maxc, pane().cur.col + 1);
}
void Editor::move_up() { if (pane().cur.row > 0) { pane().cur.row--; pane().cur.col = std::min(pane().cur.col, max_col_for_row(pane().cur.row)); } }
void Editor::move_down() { if (pane().cur.row + 1 < doc().buf.line_count()) { pane().cur.row++; pane().cur.col = std::min(pane().cur.col, max_col_for_row(pane().cur.row)); } }

void Editor::delete_char() {
  std::string s = doc().buf.line(pane().cur.row);
  if (pane().cur.col < (int)s.size()) {
    char c = s[pane().cur.col];
    s.erase(s.begin() + pane().cur.col);
    doc().buf.replace_line(pane().cur.row, s);
    push_op({Operation::DeleteChar, pane().cur.row, pane().cur.col, std::string(1, c), std::string()});
    doc().modified = true; doc().um.clear_redo();
  }
}

void Editor::delete_line() {
  reg.lines = { doc().buf.line(pane().cur.row) };
  reg.linewise = true;
  push_op({Operation::DeleteLine, pane().cur.row, 0, doc().buf.line(pane().cur.row), std::string()});
  doc().buf.erase_line(pane().cur.row);
  if (pane().cur.row >= doc().buf.line_count()) pane().cur.row = std::max(0, doc().buf.line_count() - 1);
  pane().cur.col = std::min(pane().cur.col, (int)doc().buf.line(pane().cur.row).size());
  doc().modified = true; doc().um.clear_redo();
}

void Editor::delete_lines_range(int start_row, int count) {
  if (count <= 0 || doc().buf.line_count() == 0) return;
  if (start_row < 0) start_row = 0;
  if (start_row >= doc().buf.line_count()) return;
  int max_count = doc().buf.line_count() - start_row;
  int n = std::min(count, max_count);
  reg.lines.clear();
  reg.lines.reserve(n);
  for (int i = 0; i < n; ++i) {
    const std::string& s = doc().buf.line(start_row + i);
    push_op({Operation::DeleteLine, start_row + i, 0, s, std::string()});
    reg.lines.push_back(s);
  }
  reg.linewise = true;
  doc().buf.erase_lines(start_row, start_row + n);
  pane().cur.row = std::min(start_row, std::max(0, doc().buf.line_count() - 1));
  pane().cur.col = std::min(pane().cur.col, (int)doc().buf.line(pane().cur.row).size());
  doc().modified = true; doc().um.clear_redo();
}

void Editor::reg_push_line(int row) {
  if (row < 0 || row >= doc().buf.line_count()) return;
  reg.lines.push_back(doc().buf.line(row));
  reg.linewise = true;/*paste whole line*/
}

void Editor::paste_below() {
  int insert_row = pane().cur.row + 1;
  if (reg.linewise) {
    if (reg.lines.empty()) return;
    begin_group();
    doc().buf.insert_lines(insert_row, reg.lines);
    for (size_t i = 0; i < reg.lines.size(); ++i) {
      push_op({Operation::InsertLine, insert_row + (int)i, 0, reg.lines[i], std::string()});
    }
    doc().modified = true; doc().um.clear_redo();
    commit_group();
  } else {
    if (reg.lines.empty()) return;
    const std::string& text = reg.lines[0];
    std::string s = doc().buf.line(pane().cur.row);
    int pos = std::min(pane().cur.col + 1, (int)s.size());
    size_t start = 0;
    std::vector<std::string> parts;
    while (true) {
      size_t p = text.find('\n', start);
      if (p == std::string::npos) { parts.emplace_back(text.substr(start)); break; }
      parts.emplace_back(text.substr(start, p - start)); start = p + 1;
    }
    begin_group();
    if (parts.size() == 1) {
      std::string neu = s;
      neu.insert(pos, parts[0]);
      push_op({Operation::ReplaceLine, pane().cur.row, pos, s, neu});
      doc().buf.replace_line(pane().cur.row, neu);
      doc().modified = true; doc().um.clear_redo();
    } else {
      std::string left = s.substr(0, pos);
      std::string right = s.substr(pos);
      std::string first_new = left + parts[0];
      push_op({Operation::ReplaceLine, pane().cur.row, pos, s, first_new});
      doc().buf.replace_line(pane().cur.row, first_new);
      std::vector<std::string> tail(parts.begin() + 1, parts.end());
      // Insert lines as a single block for consistent undo/redo
      if (!tail.empty()) {
        std::string block;
        for (size_t i = 0; i < tail.size(); ++i) { if (i) block.push_back('\n'); block += tail[i]; }
        push_op({Operation::InsertLinesBlock, insert_row, 0, block, std::string()});
        doc().buf.insert_lines(insert_row, tail);
      }
      // Replace last inserted line to append the original right part
      int last_row = insert_row + (int)tail.size() - 1;
      if (!tail.empty()) {
        std::string last_old = tail.back();
        std::string last_new = last_old + right;
        push_op({Operation::ReplaceLine, last_row, 0, last_old, last_new});
        doc().buf.replace_line(last_row, last_new);
      }
      doc().modified = true; doc().um.clear_redo();
    }
    commit_group();
  }
}

void Editor::insert_line_below() {
  int insert_row = pane().cur.row + 1;
  doc().buf.insert_line(insert_row, std::string());
  push_op({Operation::InsertLine, insert_row, 0, std::string(), std::string()});
  pane().cur.row = insert_row; pane().cur.col = 0; doc().modified = true; doc().um.clear_redo();
}

void Editor::insert_line_above() {
  int insert_row = pane().cur.row;
  doc().buf.insert_line(insert_row, std::string());
  push_op({Operation::InsertLine, insert_row, 0, std::string(), std::string()});
  pane().cur.col = 0; doc().modified = true; doc().um.clear_redo();
}

void Editor::split_line_at_cursor() {
  std::string s = doc().buf.line(pane().cur.row);
  std::string left = s.substr(0, pane().cur.col);
  std::string right = s.substr(pane().cur.col);
  doc().buf.replace_line(pane().cur.row, left);
  doc().buf.insert_line(pane().cur.row + 1, right);
  push_op({Operation::InsertLine, pane().cur.row + 1, 0, right, std::string()});
  pane().cur.row++; pane().cur.col = 0; doc().modified = true; doc().um.clear_redo();
}

void Editor::backspace() {
  if (pane().cur.col > 0) {
    std::string s = doc().buf.line(pane().cur.row);
    char c = s[pane().cur.col - 1];
    s.erase(s.begin() + pane().cur.col - 1);
    doc().buf.replace_line(pane().cur.row, s);
    push_op({Operation::DeleteChar, pane().cur.row, pane().cur.col - 1, std::string(1, c), std::string()});
    pane().cur.col--; doc().modified = true; doc().um.clear_redo();
  } else if (pane().cur.row > 0) {
    std::string prev = doc().buf.line(pane().cur.row - 1);
    std::string curr = doc().buf.line(pane().cur.row);
    int old_row = pane().cur.row;
    int old_col = prev.size();
    doc().buf.replace_line(pane().cur.row - 1, prev + curr);
    doc().buf.erase_line(pane().cur.row);
    push_op({Operation::ReplaceLine, old_row - 1, (int)prev.size(), prev, prev + curr});
    pane().cur.row = old_row - 1; pane().cur.col = old_col; doc().modified = true; doc().um.clear_redo();
  }
}

void Editor::undo() {
  doc().um.undo(doc().buf, pane().cur);
  doc().modified = true;
}

void Editor::redo() {
  doc().um.redo(doc().buf, pane().cur);
  doc().modified = true;
}

void Editor::move_to_top() {
  pane().cur.row = 0;
  pane().cur.col = std::min(pane().cur.col, max_col_for_row(pane().cur.row));
}

void Editor::move_to_bottom() {
  pane().cur.row = doc().buf.line_count() - 1;
  pane().cur.col = std::min(pane().cur.col, max_col_for_row(pane().cur.row));
}

/*
basicly you need to consider the following cases:
1. pane().cur.col is a space 
2. pane().cur.col is a number
3. pane().cur.col is a word
4. pane().cur.col is a symbol
*/
void Editor::move_to_next_word_left(){
  while (true) {
    const auto& line = doc().buf.line(pane().cur.row);
    int len = (int)line.size();
    if (pane().cur.col >= len) {
      if (pane().cur.row + 1 >= doc().buf.line_count()) { pane().cur.col = len; break; }
      pane().cur.row++; pane().cur.col = 0; continue;
    }
    unsigned char c = (unsigned char)line[pane().cur.col];
    if (is_space(c)) {
      while (pane().cur.col < len && is_space((unsigned char)line[pane().cur.col])) pane().cur.col++;
      if (pane().cur.col < len) { break; }
      if (pane().cur.row + 1 >= doc().buf.line_count()) { pane().cur.col = len; break; }
      pane().cur.row++; pane().cur.col = 0; continue;
    } else if (is_word(c) || is_num(c)) {
      while (pane().cur.col + 1 < len && (is_word((unsigned char)line[pane().cur.col + 1]) || is_num((unsigned char)line[pane().cur.col + 1]))) pane().cur.col++;
      if (pane().cur.col + 1 < len) { pane().cur.col++; }
      else {
        if (pane().cur.row + 1 >= doc().buf.line_count()) { pane().cur.col = len; break; }
        pane().cur.row++; pane().cur.col = 0; continue;
      }
      while (pane().cur.col < len && is_space((unsigned char)line[pane().cur.col])) pane().cur.col++;
      if (pane().cur.col < len) { break; }
      if (pane().cur.row + 1 >= doc().buf.line_count()) { pane().cur.col = len; break; }
      pane().cur.row++; pane().cur.col = 0; continue;
    } else { 
      while (pane().cur.col + 1 < len && is_symbol((unsigned char)line[pane().cur.col + 1])) pane().cur.col++;
      if (pane().cur.col + 1 < len) { pane().cur.col++; }
      else {
        if (pane().cur.row + 1 >= doc().buf.line_count()) { pane().cur.col = len; break; }
        pane().cur.row++; pane().cur.col = 0; continue;
      }
      while (pane().cur.col < len && is_space((unsigned char)line[pane().cur.col])) pane().cur.col++;
      if (pane().cur.col < len) { break; }
      if (pane().cur.row + 1 >= doc().buf.line_count()) { pane().cur.col = len; break; }
      pane().cur.row++; pane().cur.col = 0; continue;
    }
  }
}


void Editor::move_to_next_word_right() {
  while (true) {
    const auto& line = doc().buf.line(pane().cur.row);
    int len = (int)line.size();
    if (pane().cur.col >= len) {
      if (pane().cur.row + 1 >= doc().buf.line_count()) { pane().cur.col = len; break; }
      pane().cur.row++; pane().cur.col = 0; continue;
    }
    unsigned char c = (unsigned char)line[pane().cur.col];
    if (is_space(c)) {
      while (pane().cur.col < len && is_space((unsigned char)line[pane().cur.col])) pane().cur.col++;
      if (pane().cur.col >= len) {
        if (pane().cur.row + 1 >= doc().buf.line_count()) { pane().cur.col = len; break; }
        pane().cur.row++; pane().cur.col = 0; continue;
      }
      c = (unsigned char)line[pane().cur.col];
      if (is_word(c) || is_num(c)) {
        while (pane().cur.col + 1 < len && (is_word((unsigned char)line[pane().cur.col + 1]) || is_num((unsigned char)line[pane().cur.col + 1]))) pane().cur.col++;
        break;
      } else { 
        while (pane().cur.col + 1 < len && is_symbol((unsigned char)line[pane().cur.col + 1])) pane().cur.col++;
        break;
      }
    } else if (is_word(c) || is_num(c)) {
      if (pane().cur.col + 1 < len && (is_word((unsigned char)line[pane().cur.col + 1]) || is_num((unsigned char)line[pane().cur.col + 1]))) {
        while (pane().cur.col + 1 < len && (is_word((unsigned char)line[pane().cur.col + 1]) || is_num((unsigned char)line[pane().cur.col + 1]))) pane().cur.col++;
        break;
      } else {
        if (pane().cur.col + 1 < len && is_symbol((unsigned char)line[pane().cur.col + 1])) { pane().cur.col++; break; }
        pane().cur.col++;
        while (pane().cur.col < len && is_space((unsigned char)line[pane().cur.col])) pane().cur.col++;
        if (pane().cur.col >= len) {
          if (pane().cur.row + 1 >= doc().buf.line_count()) { pane().cur.col = len; break; }
          pane().cur.row++; pane().cur.col = 0;
          const auto& line2 = doc().buf.line(pane().cur.row);
          int len2 = (int)line2.size();
          while (pane().cur.col < len2 && is_space((unsigned char)line2[pane().cur.col])) pane().cur.col++;
          if (pane().cur.col < len2) {
            if (is_word((unsigned char)line2[pane().cur.col]) || is_num((unsigned char)line2[pane().cur.col])) {
              while (pane().cur.col + 1 < len2 && (is_word((unsigned char)line2[pane().cur.col + 1]) || is_num((unsigned char)line2[pane().cur.col + 1]))) pane().cur.col++;
            } else {
              while (pane().cur.col + 1 < len2 && is_symbol((unsigned char)line2[pane().cur.col + 1])) pane().cur.col++;
            }
          }
          break;
        } else {
          if (is_word((unsigned char)line[pane().cur.col]) || is_num((unsigned char)line[pane().cur.col])) {
            while (pane().cur.col + 1 < len && (is_word((unsigned char)line[pane().cur.col + 1]) || is_num((unsigned char)line[pane().cur.col + 1]))) pane().cur.col++;
          } else {
            while (pane().cur.col + 1 < len && is_symbol((unsigned char)line[pane().cur.col + 1])) pane().cur.col++;
          }
          break;
        }
      }
    } else {
      if (pane().cur.col + 1 < len && is_symbol((unsigned char)line[pane().cur.col + 1])) {
        while (pane().cur.col + 1 < len && is_symbol((unsigned char)line[pane().cur.col + 1])) pane().cur.col++; 
        break;
      } else {
        pane().cur.col++;
        continue;
      }
    }
  }
}

void Editor::move_to_previous_word_left() {
  auto isSpace = [](unsigned char c){ return std::isspace(c) != 0; };
  auto isWord  = [](unsigned char c){ return std::isalnum(c) != 0 || c == '_'; };
  while (true) {
    const auto& line = doc().buf.line(pane().cur.row);
    int len = (int)line.size();
    if (pane().cur.col == 0) {
      if (pane().cur.row == 0) { pane().cur.col = 0; break; }
      pane().cur.row--; pane().cur.col = (int)doc().buf.line(pane().cur.row).size();
      continue;
    }
    pane().cur.col--;
    unsigned char cc = (pane().cur.col < len) ? (unsigned char)line[pane().cur.col] : (unsigned char)' ';
    if (isSpace(cc)) {
      while (pane().cur.col > 0 && isSpace((unsigned char)line[pane().cur.col])) pane().cur.col--;
      if (pane().cur.col == 0) { break; }
      unsigned char c2 = (unsigned char)line[pane().cur.col];
      if (isWord(c2)) {
        while (pane().cur.col > 0 && isWord((unsigned char)line[pane().cur.col - 1])) pane().cur.col--;
        break;
      } else {
        while (pane().cur.col > 0
               && !isSpace((unsigned char)line[pane().cur.col - 1])
               && !isWord((unsigned char)line[pane().cur.col - 1])) pane().cur.col--;
        break;
      }
    } else if (isWord(cc)) {
      while (pane().cur.col > 0 && isWord((unsigned char)line[pane().cur.col - 1])) pane().cur.col--;
      break;
    } else {
      while (pane().cur.col > 0
             && !isSpace((unsigned char)line[pane().cur.col - 1])
             && !isWord((unsigned char)line[pane().cur.col - 1])) pane().cur.col--;
      break;
    }
  }
}

void Editor::move_to_beginning_of_line() {
  pane().cur.col = 0;
}

void Editor::move_to_end_of_line() {
  pane().cur.col = max_col_for_row(pane().cur.row);
}

void Editor::scroll_up_half_page() {
  int rows = term.getSize().rows;
  int max_text_rows = std::max(0, rows-1);/*the last line is command and state line,
                                               so we need to minus 1 */
  int half = std::max(1, max_text_rows / 2);
  pane().vp.top_line = std::max(0, pane().vp.top_line - half);
  pane().cur.row = std::max(0, pane().cur.row - half);
  pane().cur.col = std::min(pane().cur.col, max_col_for_row(pane().cur.row));
}

void Editor::scroll_down_half_page() {
  int rows = term.getSize().rows;
  int max_text_rows = std::max(0, rows-1);
  int half = std::max(1, max_text_rows / 2);
  int max_top = std::max(0, doc().buf.line_count() - max_text_rows);
  pane().vp.top_line = std::min(max_top, pane().vp.top_line + half);
  pane().cur.row = std::min(doc().buf.line_count() - 1, pane().cur.row + half);
  pane().cur.col = std::min(pane().cur.col, max_col_for_row(pane().cur.row));
}

void Editor::apply_operation_forward(const Operation& op, int row_delta, int col_delta) {
  auto clamp_row_existing = [&](int r) {
    int max_row = std::max(0, doc().buf.line_count() - 1);
    return std::clamp(r, 0, max_row);
  };
  auto clamp_row_insert = [&](int r) {
    int max_row = std::max(0, doc().buf.line_count());
    return std::clamp(r, 0, max_row);
  };
  switch (op.type) {
    case Operation::InsertChar: {
      int row = clamp_row_existing(op.row + row_delta);
      std::string s = doc().buf.line(row);
      int col = std::clamp(op.col + col_delta, 0, (int)s.size());
      s.insert(s.begin() + col, op.payload[0]);
      doc().buf.replace_line(row, s);
      push_op({Operation::InsertChar, row, col, std::string(1, op.payload[0]), std::string()});
      doc().modified = true;
    } break;
    case Operation::DeleteChar: {
      int row = clamp_row_existing(op.row + row_delta);
      std::string s = doc().buf.line(row);
      if (s.empty()) break;
      int col = std::clamp(op.col + col_delta, 0, (int)s.size() - 1);
      char removed = s[col];
      s.erase(s.begin() + col);
      doc().buf.replace_line(row, s);
      push_op({Operation::DeleteChar, row, col, std::string(1, removed), std::string()});
      doc().modified = true;
    } break;
    case Operation::InsertLine: {
      int row = clamp_row_insert(op.row + row_delta);
      doc().buf.insert_line(row, op.payload);
      push_op({Operation::InsertLine, row, 0, op.payload, std::string()});
      doc().modified = true;
    } break;
    case Operation::DeleteLine: {
      int row = clamp_row_existing(op.row + row_delta);
      std::string removed = doc().buf.line(row);
      doc().buf.erase_line(row);
      push_op({Operation::DeleteLine, row, 0, removed, std::string()});
      doc().modified = true;
    } break;
    case Operation::ReplaceLine: {
      int row = clamp_row_existing(op.row + row_delta);
      std::string old_line = doc().buf.line(row);
      std::string new_line = op.alt_payload;
      doc().buf.replace_line(row, new_line);
      push_op({Operation::ReplaceLine, row, op.col + col_delta, old_line, new_line});
      doc().modified = true;
    } break;
    case Operation::InsertLinesBlock: {
      int row = clamp_row_insert(op.row + row_delta);
      auto lines = split_block_lines(op.payload);
      if (lines.empty()) lines.emplace_back("");
      doc().buf.insert_lines(row, lines);
      push_op({Operation::InsertLinesBlock, row, 0, op.payload, std::string()});
      doc().modified = true;
    } break;
    case Operation::DeleteLinesBlock: {
      int row = clamp_row_existing(op.row + row_delta);
      if (row >= doc().buf.line_count()) break;
      auto source_count = static_cast<int>(split_block_lines(op.payload).size());
      if (source_count <= 0) source_count = 1;
      int end = std::min(doc().buf.line_count(), row + source_count);
      std::string block;
      for (int r = row; r < end; ++r) {
        if (r > row) block.push_back('\n');
        block += doc().buf.line(r);
      }
      if (end > row) {
        push_op({Operation::DeleteLinesBlock, row, 0, block, std::string()});
        doc().buf.erase_lines(row, end);
        doc().modified = true;
      }
    } break;
  }
}

void Editor::repeat_last_change() {
  if (!doc().last_change || doc().last_change->ops.empty()) {
    message = "no last change";
    return;
  }
  int row_delta = pane().cur.row - doc().last_change->pre.row;
  int col_delta = pane().cur.col - doc().last_change->pre.col;
  Cursor target{pane().cur.row + (doc().last_change->post.row - doc().last_change->pre.row),
                pane().cur.col + (doc().last_change->post.col - doc().last_change->pre.col)};
  begin_group();
  for (const auto& op : doc().last_change->ops) apply_operation_forward(op, row_delta, col_delta);
  doc().um.clear_redo();
  commit_group();
  pane().cur.row = std::clamp(target.row, 0, std::max(0, doc().buf.line_count() - 1));
  pane().cur.col = std::clamp(target.col, 0, max_col_for_row(pane().cur.row));
}

std::string Editor::compute_indent_for_line(int row) const {
  if (row < 0 || row >= doc().buf.line_count()) return std::string();
  std::string line = doc().buf.line(row);
  size_t i = 0;
  while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) i++;
  return line.substr(0, i);
}

void Editor::apply_indent_to_newline(const std::string& indent) {
  if (indent.empty()) return;
  int target_row = pane().cur.row;
  if (target_row < 0 || target_row >= doc().buf.line_count()) return;
  std::string line = doc().buf.line(target_row);
  if (line.rfind(indent, 0) == 0) {
    pane().cur.col = std::min<int>(indent.size(), max_col_for_row(target_row));
    return;
  }
  std::string old_line = line;
  std::string new_line = indent + line;
  doc().buf.replace_line(target_row, new_line);
  push_op({Operation::ReplaceLine, target_row, 0, old_line, new_line});
  doc().modified = true;
  pane().cur.col = std::min<int>(indent.size(), static_cast<int>(new_line.size()));
}

void Editor::indent_lines(int start_row, int count) {
  if (count <= 0) return;
  int end = std::min(doc().buf.line_count(), start_row + count);
  std::string pad(std::max(1, tab_width), ' ');
  for (int r = start_row; r < end; ++r) {
    std::string old = doc().buf.line(r);
    std::string neu = pad + old;
    push_op({Operation::ReplaceLine, r, 0, old, neu});
    doc().buf.replace_line(r, neu);
    doc().modified = true;
  }
}

void Editor::dedent_lines(int start_row, int count) {
  if (count <= 0) return;
  int end = std::min(doc().buf.line_count(), start_row + count);
  int max_cols = std::max(1, tab_width);
  for (int r = start_row; r < end; ++r) {
    std::string old = doc().buf.line(r);
    size_t remove_bytes = 0;
    int removed_cols = 0;
    while (remove_bytes < old.size() && removed_cols < max_cols) {
      char c = old[remove_bytes];
      if (c == '\t') { remove_bytes++; removed_cols += max_cols; break; }
      if (c == ' ') { remove_bytes++; removed_cols++; continue; }
      break;
    }
    if (remove_bytes == 0) continue;
    std::string neu = old.substr(remove_bytes);
    push_op({Operation::ReplaceLine, r, 0, old, neu});
    doc().buf.replace_line(r, neu);
    doc().modified = true;
  }
}

void Editor::set_tab_width(int width) {
  int w = std::max(1, width);
  tab_width = w;
  message = std::string("tabwidth=") + std::to_string(w);
}

int Editor::max_col_for_row(int row) const {
  if (row < 0 || row >= doc().buf.line_count()) return 0;
  int len = (int)doc().buf.line(row).size();
  if (virtualedit_onemore) return len;
  return (len > 0) ? (len - 1) : 0;
}
