#include "renderer.hpp"
#include <sstream>
#include <unordered_set>
#include <cctype>

void Renderer::render(ITerminal& term,
                      const TextBuffer& buf,
                      const Cursor& cur,
                      Viewport& vp,
                      Mode mode,
                      const std::optional<std::filesystem::path>& file_path,
                      bool modified,
                      const std::string& message,
                      const std::string& cmdline,
                      bool visual_active,
                      Cursor visual_anchor,
                      bool show_line_numbers,
                      bool enable_color) {
  TermSize sz = term.getSize();
  int rows = sz.rows, cols = sz.cols;
  term.clear();
  int max_text_rows = rows - 1;
  if (cur.row < vp.top_line) vp.top_line = cur.row;
  if (cur.row >= vp.top_line + max_text_rows) vp.top_line = cur.row - max_text_rows + 1;
  int ln_width = 0;
  int indent = 0;
  if (show_line_numbers) {
    int digits = 1;
    int total = std::max(1, buf.line_count());
    while (total >= 10) { total /= 10; digits++; }
    ln_width = digits;
    indent = ln_width + 1; // one space after numbers
  }
  for (int i = 0; i < max_text_rows; ++i) {
    int line_idx = vp.top_line + i;
    if (line_idx >= buf.line_count()) break;
    const auto& s = buf.line(line_idx);
    if (show_line_numbers) {
      std::string num = std::to_string(line_idx + 1);
      std::string pad(std::max(0, ln_width - (int)num.size()), ' ');
      term.drawText(i, 0, pad + num + " ");
    }
    if (visual_active) {
      int r0 = std::min(visual_anchor.row, cur.row);
      int r1 = std::max(visual_anchor.row, cur.row);
      if (mode == Mode::VisualLine) {
        if (line_idx >= r0 && line_idx <= r1) {
          term.drawHighlighted(i, indent, s, 0, (int)s.size());
        } else {
          term.drawText(i, indent, s);
        }
      } else if (mode == Mode::Visual) {
        if (line_idx == r0 && line_idx == r1) {
          int c0 = std::min(visual_anchor.col, cur.col);
          int c1 = std::max(visual_anchor.col, cur.col);
          c0 = std::max(0, std::min(c0, (int)s.size()));
          c1 = std::max(0, std::min(c1, (int)s.size()));
          term.drawHighlighted(i, indent, s, c0, c1 - c0 + 1);
        } else if (line_idx == r0) {
          int c0 = std::min(visual_anchor.col, cur.col);
          c0 = std::max(0, std::min(c0, (int)s.size()));
          term.drawHighlighted(i, indent, s, c0, (int)s.size() - c0);
        } else if (line_idx == r1) {
          int c1 = std::max(visual_anchor.col, cur.col);
          c1 = std::max(0, std::min(c1, (int)s.size()));
          term.drawHighlighted(i, indent, s, 0, c1 + 1);
        } else if (line_idx > r0 && line_idx < r1) {
          term.drawHighlighted(i, indent, s, 0, (int)s.size());
        } else {
          term.drawText(i, indent, s);
        }
      } else {
        term.drawText(i, indent, s);
      }
    }
    auto is_ascii_line = [](const std::string& t){
      for (unsigned char c : t) { if (c >= 128) return false; }
      return true;
    };
    if (!visual_active && enable_color && is_ascii_line(s)) {
      static const std::unordered_set<std::string> kwords = {
        "if", "else", "for", "while", "return", "true", "false", "null"
      };
      auto isWord = [](unsigned char c){ return std::isalnum(c) != 0 || c == '_'; };
      int cols = term.getSize().cols;
      int col = indent;
      int n = (int)s.size();
      int p = 0;
      while (p < n) {
        if (!isWord((unsigned char)s[p])) {
          int start = p;
          while (p < n && !isWord((unsigned char)s[p])) p++;
          term.drawText(i, col, s.substr(start, p - start));
          col += (p - start);
        } else {
          int start = p;
          while (p < n && isWord((unsigned char)s[p])) p++;
          std::string tok = s.substr(start, p - start);
          if (kwords.find(tok) != kwords.end()) {
            term.drawColored(i, col, tok, 1);
          } else {
            term.drawText(i, col, tok);
          }
          col += (p - start);
        }
        if (col >= cols) break;
      }
    } else if (!visual_active) {
      term.drawText(i, indent, s);
    }
  }
  std::string mode_str = (mode == Mode::Normal ? "NORMAL" : mode == Mode::Insert ? "INSERT" : mode == Mode::Command ? "COMMAND" : mode == Mode::Visual ? "VISUAL" : "VISUAL-LINE");
  std::ostringstream oss;
  oss << mode_str << "  "
      << (file_path ? file_path->string() : "[no file]")
      << (modified ? " [+]" : "")
      << "  row:" << (cur.row + 1) << " col:" << (cur.col + 1);
  if (!message.empty() && mode != Mode::Command) oss << "  | " << message;
  std::string command_str = ":";
  if(mode == Mode::Command && (cmdline.size()>0 && (cmdline[0]=='/'||cmdline[0]=='?'))){
    command_str = cmdline;
  }else{
    command_str = ":" + cmdline;
  }
  std::string status = mode == Mode::Command ? command_str : oss.str();
  if (visual_active && mode != Mode::Command) {
    status += "  | VISUAL";
    if (mode == Mode::VisualLine) status += "(LINE)";
  }
  term.drawText(rows - 1, 0, status);
  int screen_row = cur.row - vp.top_line;
  if (screen_row >= 0 && screen_row < max_text_rows) {
    int want_col = cur.col;
    int line_len = (int)buf.line(cur.row).size();
    if (want_col > line_len) want_col = line_len;
    term.moveCursor(screen_row, want_col + indent);
  } else {
    term.moveCursor(rows - 1, 0);
  }
  term.refresh();
}
