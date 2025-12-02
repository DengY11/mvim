#include <ncurses.h>
#include "editor.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include "file_reader.hpp"

static constexpr int CTRL_u = 'U'-64; 
static constexpr int CTRL_R = 'R'-64; 
static constexpr int CTRL_d = 'D'-64; 
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
static inline bool is_symble(unsigned char c) {
  return !is_space(c) && !is_word(c);
}
static int next_word_start_same_line(const std::string& line, int col) {
  int len = (int)line.size();
  if (col < 0) col = 0; if (col > len) col = len;
  if (col >= len) return len;
  unsigned char c = (unsigned char)line[col];
  if (is_space(c)) {
    while (col < len && is_space((unsigned char)line[col])) col++;
    return col;
  }
  if (is_word(c) || is_num(c)) {
    while (col + 1 < len && (is_word((unsigned char)line[col + 1]) || is_num((unsigned char)line[col + 1]))) col++;
    return std::min(len, col + 1);
  }
  while (col + 1 < len && is_symble((unsigned char)line[col + 1])) col++;
  return std::min(len, col + 1);
}
static int next_word_end_same_line(const std::string& line, int col) {
  int len = (int)line.size();
  if (col < 0) col = 0; if (col > len) col = len;
  if (col >= len) return len;
  unsigned char c = (unsigned char)line[col];
  if (is_space(c)) {
    while (col < len && is_space((unsigned char)line[col])) col++;
    if (col >= len) return len;
    unsigned char c2 = (unsigned char)line[col];
    if (is_word(c2) || is_num(c2)) {
      while (col + 1 < len && (is_word((unsigned char)line[col + 1]) || is_num((unsigned char)line[col + 1]))) col++;
      return std::min(len, col + 1);
    }
    while (col + 1 < len && is_symble((unsigned char)line[col + 1])) col++;
    return std::min(len, col + 1);
  }
  if (is_word(c) || is_num(c)) {
    while (col + 1 < len && (is_word((unsigned char)line[col + 1]) || is_num((unsigned char)line[col + 1]))) col++;
    return std::min(len, col + 1);
  }
  while (col + 1 < len && is_symble((unsigned char)line[col + 1])) col++;
  return std::min(len, col + 1);
}
Editor::Editor(const std::optional<std::filesystem::path>& file)
    : file_path(file) {
  bool ok; std::string m;
  if (file_path) {
    buf = TextBuffer::from_file(*file_path, m, ok);
    message = m;
  } else {
    buf.ensure_not_empty();
  }
  registry.register_command("w", [this](const std::vector<std::string>& args){
    std::string mm;
    if (!args.empty()) { if (buf.write_file(args[0], mm)) { modified = false; } message = mm; }
    else if (file_path) { if (buf.write_file(*file_path, mm)) { modified = false; } message = mm; }
    else { message = "don't have path, use :w <path>"; }
  });
  registry.register_command("q", [this](const std::vector<std::string>&){
    if (modified) { message = "have unsaved changes, use :w or :q! quit"; }
    else { should_quit = true; }
  });
  registry.register_command("q!", [this](const std::vector<std::string>&){ should_quit = true; });
  registry.register_command("wq", [this](const std::vector<std::string>& args){
    std::string mm;
    if (file_path) { if (buf.write_file(*file_path, mm)) { modified = false; should_quit = true; } message = mm; }
    else {
      if (!args.empty()) { if (buf.write_file(args[0], mm)) { modified = false; should_quit = true; } message = mm; }
      else { if (!modified) { should_quit = true; message = "dont have path, and no changes, quit"; } else { message = "dont have path: use :wq <path>"; } }
    }
  });
  registry.register_command("set number", [this](const std::vector<std::string>& args){
    if (args.empty()){
      if (show_line_numbers) { show_line_numbers = false; message = "number off"; }
      else { show_line_numbers = true; message = "number on"; }
    }else{
      message = "set number: use :set number on|off";
      std::string opt = args[0];
      if (opt == "on") { show_line_numbers = true; message = "number on"; }
      else if (opt == "off") { show_line_numbers = false; message = "number off"; }
      else { message = "set number: use :set number on|off"; }
    }
  });
  registry.register_command("set pair", [this](const std::vector<std::string>& args){
    if (args.empty()){
      if (auto_pair) { auto_pair = false; message = "auto-pair off"; }
      else { auto_pair = true; message = "auto-pair on"; }
    }else{
      message = "set pair: use :set pair on|off";
      std::string opt = args[0];
      if (opt == "on") { auto_pair = true; message = "auto-pair on"; }
      else if (opt == "off") { auto_pair = false; message = "auto-pair off"; }
      else { message = "set pair: use :set pair on|off"; }
    }
  });
  registry.register_command("set tabwidth", [this](const std::vector<std::string>& args){
    if (args.empty()) { message = "set tabwidth: use :set tabwidth <width>"; return; }
    const std::string& s = args[0];
    if (s.empty()) { message = "set tabwidth: width must be a number"; return; }
    bool ok = std::all_of(s.begin(), s.end(), [](unsigned char c){ return std::isdigit(c) != 0; });
    if (!ok) { message = "set tabwidth: width must be a number"; return; }
    int w = 0;
    try { w = std::stoi(s); } catch (...) { message = "set tabwidth: invalid number"; return; }
    if (w < 1) { message = "set tabwidth: width must be >= 1"; return; }
    set_tab_width(w);
  });
  registry.register_command("set color", [this](const std::vector<std::string>& args){
    if (args.empty()) { message = "set color: use :set color on|off"; return; }
    std::string v = args[0];
    if (v == "on" || v == "1" || v == "true") { enable_color = true; message = "color on"; }
    else if (v == "off" || v == "0" || v == "false") { enable_color = false; message = "color off"; }
    else { message = "set color: value must be on|off"; }
  });
  registry.register_command("set background", [this](const std::vector<std::string>& args){
    if (args.empty()) { message = "set background: use :set background default|black|white|green|yellow"; return; }
    auto to_lower = [](std::string s){ for (auto& c: s) c = (char)std::tolower((unsigned char)c); return s; };
    std::string v = to_lower(args[0]);
    short col = -1;
    if (v == "default" || v == "normal") col = -1;
    else if (v == "black") col = COLOR_BLACK;
    else if (v == "white") col = COLOR_WHITE;
    else if (v == "green") col = COLOR_GREEN;
    else if (v == "yellow") col = COLOR_YELLOW;
    else { message = "set background: unknown color"; return; }
    term.setBackground(col);
    message = std::string("background=") + v;
  });

  load_rc();
}

void Editor::run() {
  while (!should_quit) {
    render();
    int ch = getch();
    handle_input(ch);
  }
}

void Editor::begin_group() { um.beginGroup(cur); }
void Editor::commit_group() { um.commitGroup(cur); }
void Editor::push_op(const Operation& op) { um.pushOp(op); }

void Editor::render() { renderer.render(term, buf, cur, vp, mode, file_path, modified, message, cmdline, visual_active, visual_anchor, show_line_numbers, enable_color); }

void Editor::handle_input(int ch) {
  if (mode == Mode::Command) { handle_command_input(ch); return; }
  if (mode == Mode::Insert) { handle_insert_input(ch); return; }
  handle_normal_input(ch);
}

void Editor::handle_normal_input(int ch) {
  if (ch != 'd' && ch != 'y' && ch != 'g') input.reset();
  switch (ch) {
    case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
      if (input.consumeDigit(ch)) break;
      break;
    case '0':
      if (input.hasCount()) { if (input.consumeDigit('0')) break; }
      cur.col = 0; break;

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
    case 'a': begin_group(); cur.col = std::min((int)buf.line(cur.row).size(), cur.col + 1); mode = Mode::Insert; break;
    case 'o': begin_group(); insert_line_below(); mode = Mode::Insert; break;
    case 'O': begin_group(); insert_line_above(); mode = Mode::Insert; break;
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
      else if (input.consumeDd('d')) { size_t n = input.takeCount(); if (n == 0) n = 1; begin_group(); delete_lines_range(cur.row, (int)n); commit_group(); }
      else { pending_op = PendingOp::Delete; }
      break;
    case 'y':
      if (mode == Mode::Visual || mode == Mode::VisualLine) { yank_selection(); exit_visual(); }
      else if (input.consumeYy('y')) { size_t n = input.takeCount(); if (n == 0) n = 1; while (n--) yank_current_line(); }
      else { pending_op = PendingOp::Yank; }
      break;
    case 'p': paste_below(); break;
    case 'g': if (input.consumeGg('g')) {
      size_t n = input.takeCount();
      if (n == 0) { move_to_top(); }
      else {
        int target = static_cast<int>(std::max<size_t>(1, n)) - 1;
        cur.row = std::min(target, std::max(0, buf.line_count() - 1));
        cur.col = std::min(cur.col, (int)buf.line(cur.row).size());
      }
    } break;
    case 'G': {
      size_t n = input.takeCount();
      if (n == 0) { move_to_bottom(); }
      else {
        int target = static_cast<int>(std::max<size_t>(1, n)) - 1;
        cur.row = std::min(target, std::max(0, buf.line_count() - 1));
        cur.col = std::min(cur.col, (int)buf.line(cur.row).size());
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
    default: input.reset(); break;
  }
}

void Editor::handle_insert_input(int ch) {
  if (ch == ESC) { commit_group(); mode = Mode::Normal; return; }
  if (ch == KEY_BACKSPACE || ch == 127) { backspace(); return; }
  if (ch == '\t') {
    auto& s = buf.line(cur.row);
    int n = std::max(1, tab_width);
    for (int i = 0; i < n; ++i) {
      s.insert(s.begin() + std::min(cur.col, (int)s.size()), ' ');
      push_op({Operation::InsertChar, cur.row, cur.col, std::string(1, ' '), std::string()});
      modified = true;
      cur.col++;
    }
    um.clear_redo();
    return;
  }
  if (ch == '('|| ch == '{'||ch == '[') {
    if (auto_pair) {
      insert_pair(ch);
      return;
    }
  }
  if (ch == '\n' || ch == KEY_ENTER || ch == '\r') { split_line_at_cursor(); return; }
  if (ch >= 32 && ch <= 126) {
    auto& s = buf.line(cur.row);
    if (cur.col < 0) cur.col = 0;
    if (cur.col > (int)s.size()) cur.col = (int)s.size();
    s.insert(s.begin() + cur.col, (char)ch);
    push_op({Operation::InsertChar, cur.row, cur.col, std::string(1, (char)ch), std::string()});
    modified = true;
    cur.col++;
    um.clear_redo();
    return;
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
  auto& s = buf.line(cur.row);
  if (cur.col < 0) cur.col = 0;
  if (cur.col > (int)s.size()) cur.col = (int)s.size();
  s.insert(s.begin() + cur.col, (char)ch);
  push_op({Operation::InsertChar, cur.row, cur.col, std::string(1, (char)ch), std::string()});
  modified = true;
  cur.col++;
  if (!(cur.col < (int)s.size() && s[cur.col] == closing)) {
    s.insert(s.begin() + cur.col, closing);
    push_op({Operation::InsertChar, cur.row, cur.col, std::string(1, closing), std::string()});
    modified = true;
  }
  um.clear_redo();
}

void Editor::handle_command_input(int ch) {
  if (ch == ESC) { mode = Mode::Normal; return; }
  if (ch == KEY_BACKSPACE || ch == 127) { if (!cmdline.empty()) cmdline.pop_back(); return; }
  if (ch == '\n' || ch == KEY_ENTER || ch == '\r') { execute_command(); mode = Mode::Normal; return; }
  if (ch >= 32 && ch <= 126) { cmdline.push_back((char)ch); }
}

void Editor::execute_command() {
  if (!cmdline.empty() && (cmdline[0] == '/' || cmdline[0] == '?')) {
    std::string pat = cmdline.substr(1);
    last_search = pat;
    last_search_forward = (cmdline[0] == '/');
    if (last_search_forward) search_forward(pat); else search_backward(pat);
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
  Cursor old = cur;
  const auto& s = buf.line(old.row);
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
  buf.line(old.row) = neu;
  cur.row = old.row; cur.col = c0;
  modified = true; um.clear_redo();
  commit_group();
}

void Editor::yank_to_next_word() {
  Cursor old = cur;
  const auto& s = buf.line(old.row);
  int end_col = next_word_start_same_line(s, old.col);
  if (end_col <= old.col) return;
  int c0 = std::clamp(old.col, 0, (int)s.size());
  int c1 = std::clamp(end_col, 0, (int)s.size());
  reg.lines.clear(); reg.linewise = false;
  reg.lines = { s.substr(c0, c1 - c0) };
}

void Editor::delete_to_word_end() {
  Cursor old = cur;
  const auto& s = buf.line(old.row);
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
  buf.line(old.row) = neu;
  cur.row = old.row; cur.col = c0;
  modified = true; um.clear_redo();
  commit_group();
}

void Editor::yank_to_word_end() {
  Cursor old = cur;
  const auto& s = buf.line(old.row);
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
  if (!mmapReadLines(p, lines, msg)) { message = msg; return; }
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
  int rows = buf.line_count();
  {
    const auto& line = buf.line(cur.row);
    int pos = kmp_find_first_from(line, pattern, (size_t)std::min((int)line.size(), cur.col + 1));
    if (pos >= 0) { cur.col = pos; return; }
  }
  for (int r = cur.row + 1; r < rows; ++r) {
    const auto& s = buf.line(r);
    int pos = kmp_find_first_from(s, pattern, 0);
    if (pos >= 0) { cur.row = r; cur.col = pos; return; }
  }
  message = "not found pattern";
}

void Editor::search_backward(const std::string& pattern) {
  if (pattern.empty()) { message = "pattern empty"; return; }
  {
    const auto& line = buf.line(cur.row);
    std::vector<int> pos;
    kmp_find_all(line, pattern, pos);
    int target = -1; for (int p : pos) if (p < cur.col) target = p; 
    if (target >= 0) { cur.col = target; return; }
  }
  for (int r = cur.row - 1; r >= 0; --r) {
    const auto& s = buf.line(r);
    std::vector<int> pos;
    kmp_find_all(s, pattern, pos);
    if (!pos.empty()) { cur.row = r; cur.col = pos.back(); return; }
  }
  message = "not found pattern";
}

void Editor::repeat_last_search(bool is_forward) {
  if (last_search.empty()) { message = "no last search"; return; }
  if (is_forward) search_forward(last_search); else search_backward(last_search);
}

void Editor::enter_visual_char() {
  visual_active = true;
  visual_anchor = cur;
  mode = Mode::Visual;
}

void Editor::enter_visual_line() {
  visual_active = true;
  visual_anchor = {cur.row, 0};
  mode = Mode::VisualLine;
}

void Editor::exit_visual() {
  visual_active = false;
  mode = Mode::Normal;
}

void Editor::get_visual_range(int& r0, int& r1, int& c0, int& c1) const {
  r0 = std::min(visual_anchor.row, cur.row);
  r1 = std::max(visual_anchor.row, cur.row);
  if (mode == Mode::VisualLine) {
    c0 = 0; c1 = -1; 
  } else {
    int a = visual_anchor.col;
    int b = cur.col;
    c0 = std::min(a, b);
    c1 = std::max(a, b) + 1; 
  }
}

void Editor::delete_selection() {
  int r0, r1, c0, c1; get_visual_range(r0, r1, c0, c1);
  begin_group();
  yank_selection();
  if (mode == Mode::VisualLine) {
    int end = std::min(r1 + 1, buf.line_count());
    std::string block;
    for (int k = r0; k <= std::min(r1, buf.line_count() - 1); ++k) {
      block += buf.line(k);
      if (k < r1) block += '\n';
    }
    push_op({Operation::DeleteLinesBlock, r0, 0, block, std::string()});
    if (r0 < end) buf.lines.erase(buf.lines.begin() + r0, buf.lines.begin() + end);
    if (buf.empty()) buf.lines.emplace_back("");
    cur.row = std::min(r0, buf.line_count() - 1);
    cur.col = 0;
  } else {
    if (r0 == r1) {
      auto& s = buf.line(r0);
      c0 = std::clamp(c0, 0, (int)s.size());
      c1 = std::clamp(c1, 0, (int)s.size());
      if (c1 > c0) {
        std::string old = s;
        std::string neu = old.substr(0, c0) + old.substr(c1);
        push_op({Operation::ReplaceLine, r0, c0, old, neu});
        s = neu;
      }
      cur.row = r0; cur.col = c0;
    } else {
      auto& first = buf.line(r0);
      auto& last = buf.line(r1);
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
          mid += buf.line(k);
          if (k < r1) mid += '\n';
        }
        push_op({Operation::DeleteLinesBlock, r0 + 1, 0, mid, std::string()});
      }
      first = neu_first;
      buf.lines.erase(buf.lines.begin() + (r0 + 1), buf.lines.begin() + (r1 + 1));
      cur.row = r0; cur.col = (int)left.size();
    }
  }
  modified = true; um.clear_redo();
  commit_group();
}

void Editor::yank_selection() {
  int r0, r1, c0, c1; get_visual_range(r0, r1, c0, c1);
  reg.lines.clear();
  if (mode == Mode::VisualLine) {
    for (int r = r0; r <= r1; ++r) reg.lines.push_back(buf.line(r));
    reg.linewise = true;
  } else {
    if (r0 == r1) {
      const auto& s = buf.line(r0);
      c0 = std::clamp(c0, 0, (int)s.size());
      c1 = std::clamp(c1, 0, (int)s.size());
      if (c1 > c0) reg.lines = { s.substr(c0, c1 - c0) }; else reg.lines = { "" };
      reg.linewise = false;
    } else {
      const auto& first = buf.line(r0);
      const auto& last = buf.line(r1);
      c0 = std::clamp(c0, 0, (int)first.size());
      c1 = std::clamp(c1, 0, (int)last.size());
      std::string out;
      out += first.substr(c0);
      out += '\n';
      for (int r = r0 + 1; r <= r1 - 1; ++r) { out += buf.line(r); out += '\n'; }
      out += last.substr(0, c1);
      reg.lines = { out };
      reg.linewise = false;
    }
  }
}

void Editor::move_left() { if (cur.col > 0) cur.col--; }
void Editor::move_right() { cur.col = std::min((int)buf.line(cur.row).size(), cur.col + 1); }
void Editor::move_up() { if (cur.row > 0) { cur.row--; cur.col = std::min(cur.col, (int)buf.line(cur.row).size()); } }
void Editor::move_down() { if (cur.row + 1 < buf.line_count()) { cur.row++; cur.col = std::min(cur.col, (int)buf.line(cur.row).size()); } }

void Editor::delete_char() {
  auto& s = buf.line(cur.row);
  if (cur.col < (int)s.size()) {
    char c = s[cur.col];
    s.erase(s.begin() + cur.col);
    push_op({Operation::DeleteChar, cur.row, cur.col, std::string(1, c), std::string()});
    modified = true; um.clear_redo();
  }
}

void Editor::delete_line() {
  reg.lines = { buf.line(cur.row) };
  reg.linewise = true;
  push_op({Operation::DeleteLine, cur.row, 0, buf.line(cur.row), std::string()});
  buf.lines.erase(buf.lines.begin() + cur.row);
  if (cur.row >= buf.line_count()) cur.row = std::max(0, buf.line_count() - 1);
  if (buf.empty()) buf.lines.emplace_back(""); cur.col = std::min(cur.col, (int)buf.line(cur.row).size());
  modified = true; um.clear_redo();
}

void Editor::delete_lines_range(int start_row, int count) {
  if (count <= 0 || buf.line_count() == 0) return;
  if (start_row < 0) start_row = 0;
  if (start_row >= buf.line_count()) return;
  int max_count = buf.line_count() - start_row;
  int n = std::min(count, max_count);
  reg.lines.clear();
  reg.lines.reserve(n);
  for (int i = 0; i < n; ++i) {
    const std::string& s = buf.line(start_row + i);
    push_op({Operation::DeleteLine, start_row + i, 0, s, std::string()});
    reg.lines.push_back(s);
  }
  reg.linewise = true;
  buf.lines.erase(buf.lines.begin() + start_row, buf.lines.begin() + start_row + n);
  if (buf.empty()) buf.lines.emplace_back("");
  cur.row = std::min(start_row, std::max(0, buf.line_count() - 1));
  cur.col = std::min(cur.col, (int)buf.line(cur.row).size());
  modified = true; um.clear_redo();
}

void Editor::yank_current_line() { reg.lines = { buf.line(cur.row) }; reg.linewise = true; }

void Editor::paste_below() {
  int insert_row = cur.row + 1;
  if (reg.linewise) {
    if (reg.lines.empty()) return;
    begin_group();
    buf.lines.insert(buf.lines.begin() + insert_row, reg.lines.begin(), reg.lines.end());
    for (size_t i = 0; i < reg.lines.size(); ++i) {
      push_op({Operation::InsertLine, insert_row + (int)i, 0, reg.lines[i], std::string()});
    }
    modified = true; um.clear_redo();
    commit_group();
  } else {
    if (reg.lines.empty()) return;
    const std::string& text = reg.lines[0];
    auto& s = buf.line(cur.row);
    int pos = std::min(cur.col + 1, (int)s.size());
    size_t start = 0;
    std::vector<std::string> parts;
    while (true) {
      size_t p = text.find('\n', start);
      if (p == std::string::npos) { parts.emplace_back(text.substr(start)); break; }
      parts.emplace_back(text.substr(start, p - start)); start = p + 1;
    }
    begin_group();
    if (parts.size() == 1) {
      s.insert(pos, parts[0]);
      push_op({Operation::ReplaceLine, cur.row, pos, s.substr(0, pos), s});
      modified = true; um.clear_redo();
    } else {
      std::string left = s.substr(0, pos);
      std::string right = s.substr(pos);
      s = left + parts[0];
      buf.lines.insert(buf.lines.begin() + insert_row, parts.begin() + 1, parts.end());
      push_op({Operation::InsertLine, insert_row, 0, parts[1], std::string()});
      for (size_t i = 2; i < parts.size(); ++i) {
        push_op({Operation::InsertLine, insert_row + (int)i - 1, 0, parts[i], std::string()});
      }
      auto& last = buf.line(insert_row + (int)parts.size() - 1);
      last += right;
      modified = true; um.clear_redo();
    }
    commit_group();
  }
}

void Editor::insert_line_below() {
  int insert_row = cur.row + 1;
  buf.lines.insert(buf.lines.begin() + insert_row, std::string());
  push_op({Operation::InsertLine, insert_row, 0, std::string(), std::string()});
  cur.row = insert_row; cur.col = 0; modified = true; um.clear_redo();
}

void Editor::insert_line_above() {
  int insert_row = cur.row;
  buf.lines.insert(buf.lines.begin() + insert_row, std::string());
  push_op({Operation::InsertLine, insert_row, 0, std::string(), std::string()});
  cur.col = 0; modified = true; um.clear_redo();
}

void Editor::split_line_at_cursor() {
  auto& s = buf.line(cur.row);
  std::string left = s.substr(0, cur.col);
  std::string right = s.substr(cur.col);
  s = left;
  buf.lines.insert(buf.lines.begin() + cur.row + 1, right);
  push_op({Operation::InsertLine, cur.row + 1, 0, right, std::string()});
  cur.row++; cur.col = 0; modified = true; um.clear_redo();
}

void Editor::backspace() {
  if (cur.col > 0) {
    auto& s = buf.line(cur.row);
    char c = s[cur.col - 1];
    s.erase(s.begin() + cur.col - 1);
    push_op({Operation::DeleteChar, cur.row, cur.col - 1, std::string(1, c), std::string()});
    cur.col--; modified = true; um.clear_redo();
  } else if (cur.row > 0) {
    std::string prev = buf.line(cur.row - 1);
    std::string curr = buf.line(cur.row);
    int old_row = cur.row;
    int old_col = prev.size();
    buf.line(cur.row - 1) = prev + curr;
    buf.lines.erase(buf.lines.begin() + cur.row);
    push_op({Operation::ReplaceLine, old_row - 1, (int)prev.size(), prev, prev + curr});
    cur.row = old_row - 1; cur.col = old_col; modified = true; um.clear_redo();
  }
}

void Editor::undo() {
  um.undo(buf, cur);
  modified = true;
}

void Editor::redo() {
  um.redo(buf, cur);
  modified = true;
}

void Editor::move_to_top() {
  cur.row = 0;
  cur.col = std::min(cur.col, (int)buf.line(cur.row).size());
}

void Editor::move_to_bottom() {
  cur.row = buf.line_count() - 1;
  cur.col = std::min(cur.col, (int)buf.line(cur.row).size());
}

/*
basicly you need to consider the following cases:
1. cur.col is a space 
2. cur.col is a number
3. cur.col is a word
4. cur.col is a symbol
*/
void Editor::move_to_next_word_left(){
  while (true) {
    const auto& line = buf.line(cur.row);
    int len = (int)line.size();
    if (cur.col >= len) {
      if (cur.row + 1 >= buf.line_count()) { cur.col = len; break; }
      cur.row++; cur.col = 0; continue;
    }
    unsigned char c = (unsigned char)line[cur.col];
    if (is_space(c)) {
      while (cur.col < len && is_space((unsigned char)line[cur.col])) cur.col++;
      if (cur.col < len) { break; }
      if (cur.row + 1 >= buf.line_count()) { cur.col = len; break; }
      cur.row++; cur.col = 0; continue;
    } else if (is_word(c) || is_num(c)) {
      while (cur.col + 1 < len && (is_word((unsigned char)line[cur.col + 1]) || is_num((unsigned char)line[cur.col + 1]))) cur.col++;
      if (cur.col + 1 < len) { cur.col++; }
      else {
        if (cur.row + 1 >= buf.line_count()) { cur.col = len; break; }
        cur.row++; cur.col = 0; continue;
      }
      while (cur.col < len && is_space((unsigned char)line[cur.col])) cur.col++;
      if (cur.col < len) { break; }
      if (cur.row + 1 >= buf.line_count()) { cur.col = len; break; }
      cur.row++; cur.col = 0; continue;
    } else { 
      while (cur.col + 1 < len && is_symble((unsigned char)line[cur.col + 1])) cur.col++;
      if (cur.col + 1 < len) { cur.col++; }
      else {
        if (cur.row + 1 >= buf.line_count()) { cur.col = len; break; }
        cur.row++; cur.col = 0; continue;
      }
      while (cur.col < len && is_space((unsigned char)line[cur.col])) cur.col++;
      if (cur.col < len) { break; }
      if (cur.row + 1 >= buf.line_count()) { cur.col = len; break; }
      cur.row++; cur.col = 0; continue;
    }
  }
}


void Editor::move_to_next_word_right() {
  while (true) {
    const auto& line = buf.line(cur.row);
    int len = (int)line.size();
    if (cur.col >= len) {
      if (cur.row + 1 >= buf.line_count()) { cur.col = len; break; }
      cur.row++; cur.col = 0; continue;
    }
    unsigned char c = (unsigned char)line[cur.col];
    if (is_space(c)) {
      while (cur.col < len && is_space((unsigned char)line[cur.col])) cur.col++;
      if (cur.col >= len) {
        if (cur.row + 1 >= buf.line_count()) { cur.col = len; break; }
        cur.row++; cur.col = 0; continue;
      }
      c = (unsigned char)line[cur.col];
      if (is_word(c) || is_num(c)) {
        while (cur.col + 1 < len && (is_word((unsigned char)line[cur.col + 1]) || is_num((unsigned char)line[cur.col + 1]))) cur.col++;
        break;
      } else { 
        while (cur.col + 1 < len && is_symble((unsigned char)line[cur.col + 1])) cur.col++;
        break;
      }
    } else if (is_word(c) || is_num(c)) {
      if (cur.col + 1 < len && (is_word((unsigned char)line[cur.col + 1]) || is_num((unsigned char)line[cur.col + 1]))) {
        while (cur.col + 1 < len && (is_word((unsigned char)line[cur.col + 1]) || is_num((unsigned char)line[cur.col + 1]))) cur.col++;
        break;
      } else {
        if (cur.col + 1 < len && is_symble((unsigned char)line[cur.col + 1])) { cur.col++; break; }
        cur.col++;
        while (cur.col < len && is_space((unsigned char)line[cur.col])) cur.col++;
        if (cur.col >= len) {
          if (cur.row + 1 >= buf.line_count()) { cur.col = len; break; }
          cur.row++; cur.col = 0;
          const auto& line2 = buf.line(cur.row);
          int len2 = (int)line2.size();
          while (cur.col < len2 && is_space((unsigned char)line2[cur.col])) cur.col++;
          if (cur.col < len2) {
            if (is_word((unsigned char)line2[cur.col]) || is_num((unsigned char)line2[cur.col])) {
              while (cur.col + 1 < len2 && (is_word((unsigned char)line2[cur.col + 1]) || is_num((unsigned char)line2[cur.col + 1]))) cur.col++;
            } else {
              while (cur.col + 1 < len2 && is_symble((unsigned char)line2[cur.col + 1])) cur.col++;
            }
          }
          break;
        } else {
          if (is_word((unsigned char)line[cur.col]) || is_num((unsigned char)line[cur.col])) {
            while (cur.col + 1 < len && (is_word((unsigned char)line[cur.col + 1]) || is_num((unsigned char)line[cur.col + 1]))) cur.col++;
          } else {
            while (cur.col + 1 < len && is_symble((unsigned char)line[cur.col + 1])) cur.col++;
          }
          break;
        }
      }
    } else {
      if (cur.col + 1 < len && is_symble((unsigned char)line[cur.col + 1])) {
        while (cur.col + 1 < len && is_symble((unsigned char)line[cur.col + 1])) cur.col++;
        break;
      } else {
        cur.col++;
        continue;
      }
    }
  }
}

void Editor::move_to_previous_word_left() {
  auto isSpace = [](unsigned char c){ return std::isspace(c) != 0; };
  auto isWord  = [](unsigned char c){ return std::isalnum(c) != 0 || c == '_'; };
  while (true) {
    const auto& line = buf.line(cur.row);
    int len = (int)line.size();
    if (cur.col == 0) {
      if (cur.row == 0) { cur.col = 0; break; }
      cur.row--; cur.col = (int)buf.line(cur.row).size();
      continue;
    }
    cur.col--;
    unsigned char cc = (cur.col < len) ? (unsigned char)line[cur.col] : (unsigned char)' ';
    if (isSpace(cc)) {
      while (cur.col > 0 && isSpace((unsigned char)line[cur.col])) cur.col--;
      if (cur.col == 0) { break; }
      unsigned char c2 = (unsigned char)line[cur.col];
      if (isWord(c2)) {
        while (cur.col > 0 && isWord((unsigned char)line[cur.col - 1])) cur.col--;
        break;
      } else {
        while (cur.col > 0
               && !isSpace((unsigned char)line[cur.col - 1])
               && !isWord((unsigned char)line[cur.col - 1])) cur.col--;
        break;
      }
    } else if (isWord(cc)) {
      while (cur.col > 0 && isWord((unsigned char)line[cur.col - 1])) cur.col--;
      break;
    } else {
      while (cur.col > 0
             && !isSpace((unsigned char)line[cur.col - 1])
             && !isWord((unsigned char)line[cur.col - 1])) cur.col--;
      break;
    }
  }
}

void Editor::move_to_beginning_of_line() {
  cur.col = 0;
}

void Editor::move_to_end_of_line() {
  cur.col = (int)buf.line(cur.row).size();
}

void Editor::scroll_up_half_page() {
  int rows = term.getSize().rows;
  int max_text_rows = std::max(0, rows-1);/*the last line is command and state line,
                                               so we need to minus 1 */
  int half = std::max(1, max_text_rows / 2);
  vp.top_line = std::max(0, vp.top_line - half);
  cur.row = std::max(0, cur.row - half);
  cur.col = std::min(cur.col, (int)buf.line(cur.row).size());
}

void Editor::scroll_down_half_page() {
  int rows = term.getSize().rows;
  int max_text_rows = std::max(0, rows-1);
  int half = std::max(1, max_text_rows / 2);
  int max_top = std::max(0, buf.line_count() - max_text_rows);
  vp.top_line = std::min(max_top, vp.top_line + half);
  cur.row = std::min(buf.line_count() - 1, cur.row + half);
  cur.col = std::min(cur.col, (int)buf.line(cur.row).size());
}

void Editor::set_tab_width(int width) {
  int w = std::max(1, width);
  tab_width = w;
  message = std::string("tabwidth=") + std::to_string(w);
}
