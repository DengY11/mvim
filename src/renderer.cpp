#include "renderer.hpp"
#include <sstream>
#include <unordered_set>
#include <cctype>
#include <string>
#include <algorithm>

static std::string toLower(std::string s){ for(char& c: s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; }
static const std::unordered_set<std::string>& keywordsForExt(const std::string& ext){
  static const std::unordered_set<std::string> def = {"if","else","for","while","return","true","false","null"};
  static const std::unordered_set<std::string> cxx = {
    "if","else","for","while","return","switch","case","break","continue","struct","class","public","private","protected","const","static","enum","sizeof","typedef","namespace","using","new","delete","true","false","null","include","void","int","char","float","double","long","short","signed","unsigned"
  };
  static const std::unordered_set<std::string> go = {
    "package","import","func","var","const","type","struct","interface","if","else","for","range","return","go","defer","select","switch","case","map","chan","true","false","nil"
  };
  static const std::unordered_set<std::string> bash = {
    "if","then","else","elif","fi","for","in","do","done","case","esac","function","local","export"
  };
  std::string e = toLower(ext);
  if(e=="c") return cxx;
  if(e=="cpp"||e=="cxx"||e=="cc"||e=="h"||e=="hpp"||e=="hxx") return cxx;
  if(e=="go") return go;
  if(e=="bash") return bash;
  return def;
}

static void render_welcome_mvim(ITerminal& term, int rows, int cols, int indent){
  const char* art[] = {
    "MM   MM   V     V    I    MM   MM",
    "M M M M    V   V     I    M M M M",
    "M  M  M     V V      I    M  M  M",
    "M     M      V     III    M     M"
  };
  int lines = 4;
  int max_text_rows = std::max(0, rows - 1);
  int text_cols = std::max(0, cols - indent);
  int start_row = std::max(0, (max_text_rows - lines) / 2);
  int max_len = 0;
  for(int i=0;i<lines;i++){ int len = static_cast<int>(std::strlen(art[i])); if(len>max_len) max_len = len; }
  int start_col = indent + std::max(0, (text_cols - max_len) / 2);
  for(int i=0;i<lines;i++){
    term.draw_text(start_row + i, start_col, std::string(art[i]));
    term.clear_to_eol(start_row + i, start_col + static_cast<int>(std::strlen(art[i])));
  }
}

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
                      bool enable_color,
                      const std::vector<SearchHit>& search_hits) {
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
  std::string ext;
  if (file_path) {
    auto e = file_path->extension().string();
    if (!e.empty() && e[0]=='.') e = e.substr(1);
    ext = toLower(e);
  }
  const auto& kwords_global = keywordsForExt(ext);
  int text_cols = std::max(0, cols - indent);
  if (text_cols <= 0) vp.left_col = 0;
  if (text_cols > 0) {
    if (cur.col < vp.left_col) vp.left_col = cur.col;
    else if (cur.col >= vp.left_col + text_cols) vp.left_col = cur.col - text_cols + 1;
    if (vp.left_col < 0) vp.left_col = 0;
  }
  bool show_welcome = (!file_path && buf.line_count() == 1 && buf.line(0).empty());
  if (show_welcome) {
    render_welcome_mvim(term, rows, cols, indent);
  } else {
  for (int i = 0; i < max_text_rows; ++i) {
    int line_idx = vp.top_line + i;
    if (line_idx >= buf.line_count()) break;
    const auto& s = buf.line(line_idx);
    int s_len = static_cast<int>(s.size());
    int start_col = std::min(std::max(0, vp.left_col), s_len);
    int end_col = std::min(s_len, start_col + std::max(0, text_cols));
    if (show_line_numbers) {
      std::string num = std::to_string(line_idx + 1);
      std::string pad(std::max(0, ln_width - static_cast<int>(num.size())), ' ');
      term.draw_text(i, 0, pad + num + " ");
    }
    if (visual_active) {
      int r0 = std::min(visual_anchor.row, cur.row);
      int r1 = std::max(visual_anchor.row, cur.row);
      if (mode == Mode::VisualLine) {
        if (line_idx >= r0 && line_idx <= r1) {
          std::string vis = s.substr(start_col, end_col - start_col);
          term.draw_highlighted(i, indent, vis, 0, static_cast<int>(vis.size()));
          term.clear_to_eol(i, indent + static_cast<int>(vis.size()));
        } else {
          std::string vis = s.substr(start_col, end_col - start_col);
          term.draw_text(i, indent, vis);
          term.clear_to_eol(i, indent + static_cast<int>(vis.size()));
        }
      } else if (mode == Mode::Visual) {
        auto is_ascii_line = [](const std::string& t){ for (unsigned char c : t) { if (c >= 128) return false; } return true; };
        std::string vis = s.substr(start_col, end_col - start_col);
        if (!is_ascii_line(vis)) {
          if (line_idx >= r0 && line_idx <= r1) term.draw_highlighted(i, indent, vis, 0, (int)vis.size()); else term.draw_text(i, indent, vis);
          term.clear_to_eol(i, indent + (int)vis.size());
        } else {
        if (line_idx == r0 && line_idx == r1) {
          int c0 = std::min(visual_anchor.col, cur.col);
          int c1 = std::max(visual_anchor.col, cur.col);
          c0 = std::max(0, std::min(c0, static_cast<int>(s.size())));
          c1 = std::max(0, std::min(c1, static_cast<int>(s.size())));
          int hs = std::max(0, c0 - start_col);
          int he = std::max(0, std::min(c1, end_col) - start_col);
          int hlen = std::max(0, he - hs);
          term.draw_highlighted(i, indent, vis, hs, hlen);
        } else if (line_idx == r0) {
          int c0 = std::min(visual_anchor.col, cur.col);
          c0 = std::max(0, std::min(c0, static_cast<int>(s.size())));
          int hs = std::max(0, c0 - start_col);
          int hlen = std::max(0, end_col - std::max(start_col, c0));
          term.draw_highlighted(i, indent, vis, hs, hlen);
        } else if (line_idx == r1) {
          int c1 = std::max(visual_anchor.col, cur.col);
          c1 = std::max(0, std::min(c1, static_cast<int>(s.size())));
          int hlen = std::max(0, std::min(c1, end_col) - start_col);
          term.draw_highlighted(i, indent, vis, 0, hlen);
        } else if (line_idx > r0 && line_idx < r1) {
          term.draw_highlighted(i, indent, vis, 0, static_cast<int>(vis.size()));
        } else {
          term.draw_text(i, indent, vis);
        }
        term.clear_to_eol(i, indent + (int)vis.size());
        }
      } else {
        term.draw_text(i, indent, s.substr(start_col, end_col - start_col));
      }
    }
    auto is_ascii_line = [](const std::string& t){
      for (unsigned char c : t) { if (c >= 128) return false; }
      return true;
    };
    std::string vis_line = s.substr(start_col, end_col - start_col);
    // Search highlight (disable during visual selection)
    auto draw_with_search_highlight = [&](const std::string& full_line, int row_screen, int start_col_full, int end_col_full){
      // collect hits in this line
      std::vector<SearchHit> hits;
      for (const auto& h : search_hits) if (h.row == line_idx) hits.push_back(h);
      // filter visible range
      std::vector<SearchHit> vis_hits;
      for (const auto& h : hits) {
        int h_start = h.col;
        int h_end = h.col + h.len;
        if (h_end <= start_col_full || h_start >= end_col_full) continue;
        int vis_start = std::max(h_start, start_col_full) - start_col_full;
        int vis_len = std::min(h_end, end_col_full) - std::max(h_start, start_col_full);
        if (vis_len > 0) vis_hits.push_back({line_idx, vis_start, vis_len});
      }
      if (vis_hits.empty()) {
        term.draw_text(row_screen, indent, vis_line);
        term.clear_to_eol(row_screen, indent + (int)vis_line.size());
        return;
      }
      int col_draw = indent;
      int cursor = 0;
      // sort by start
      std::sort(vis_hits.begin(), vis_hits.end(), [](const SearchHit& a, const SearchHit& b){ return a.col < b.col; });
      for (const auto& h : vis_hits) {
        if (h.col > cursor) {
          term.draw_text(row_screen, col_draw, vis_line.substr(cursor, h.col - cursor));
          col_draw += (h.col - cursor);
          cursor = h.col;
        }
        int hl_len = h.len;
        term.draw_highlighted(row_screen, col_draw, vis_line.substr(cursor, hl_len), 0, hl_len);
        col_draw += hl_len;
        cursor += hl_len;
      }
      if (cursor < (int)vis_line.size()) {
        term.draw_text(row_screen, col_draw, vis_line.substr(cursor));
        col_draw += (int)vis_line.size() - cursor;
      }
      term.clear_to_eol(row_screen, col_draw);
    };

    if (!visual_active && !search_hits.empty()) {
      draw_with_search_highlight(s, i, start_col, end_col);
    } else if (!visual_active && enable_color && is_ascii_line(vis_line)) {
      auto isWord = [](unsigned char c){ return std::isalnum(c) != 0 || c == '_'; };
      int cols = term.getSize().cols;
      int col = indent;
      int n = static_cast<int>(vis_line.size());
      int p = 0;
      while (p < n) {
        if (!isWord(static_cast<unsigned char>(vis_line[p]))) {
          int start = p;
          while (p < n && !isWord(static_cast<unsigned char>(vis_line[p]))) p++;
          term.draw_text(i, col, vis_line.substr(start, p - start));
          col += (p - start);
        } else {
          int start = p;
          while (p < n && isWord(static_cast<unsigned char>(vis_line[p]))) p++;
          std::string tok = vis_line.substr(start, p - start);
          if (kwords_global.find(tok) != kwords_global.end()) {
            term.draw_colored(i, col, tok, 1);
          } else {
            term.draw_text(i, col, tok);
          }
          col += (p - start);
        }
        if (col >= cols) break;
      }
    } else if (!visual_active) {
      term.draw_text(i, indent, vis_line);
      term.clear_to_eol(i, indent + static_cast<int>(vis_line.size()));
    }
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
  term.draw_text(rows - 1, 0, status);
  int screen_row = cur.row - vp.top_line;
  if (screen_row >= 0 && screen_row < max_text_rows) {
    int want_col = cur.col;
  int line_len = static_cast<int>(buf.line(cur.row).size());
    if (want_col > line_len) want_col = line_len;
    int screen_col = indent + std::max(0, want_col - std::max(0, vp.left_col));
    screen_col = std::min(screen_col, cols - 1);
    term.move_cursor(screen_row, screen_col);
  } else {
    term.move_cursor(rows - 1, 0);
  }
  term.refresh();
}
