#include "editor.hpp"
#include <ncurses.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <filesystem>

#define buf (doc().buf)
#define modified (doc().modified)
#define file_path (doc().file_path)

void Editor::register_commands() {
  registry.register_command("w", [this](const std::vector<std::string>& args){
    std::string mm;
    if (!args.empty()) { if (buf.write_file(args[0], mm)) { modified = false; } message = mm; }
    else if (file_path) { if (buf.write_file(*file_path, mm)) { modified = false; } message = mm; }
    else { message = "don't have path, use :w <path>"; }
  });
  registry.register_command("q", [this](const std::vector<std::string>&){
    close_or_quit(false);
  });
  registry.register_command("q!", [this](const std::vector<std::string>&){ close_or_quit(true); });
  registry.register_command("wq", [this](const std::vector<std::string>& args){
    std::string mm;
    if (file_path) { if (buf.write_file(*file_path, mm)) { modified = false; close_or_quit(true); } message = mm; }
    else {
      if (!args.empty()) { if (buf.write_file(args[0], mm)) { modified = false; close_or_quit(true); } message = mm; }
      else { if (!modified) { close_or_quit(true); message = "dont have path, and no changes, quit"; } else { message = "dont have path: use :wq <path>"; } }
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
  registry.register_command("set relativenumber", [this](const std::vector<std::string>& args){
    if (args.empty()){
      relative_line_numbers = !relative_line_numbers;
      message = relative_line_numbers ? "relativenumber on" : "relativenumber off";
    } else {
      message = "set relativenumber: use :set relativenumber on|off";
      std::string opt = args[0];
      if (opt == "on") { relative_line_numbers = true; message = "relativenumber on"; }
      else if (opt == "off") { relative_line_numbers = false; message = "relativenumber off"; }
      else { message = "set relativenumber: use :set relativenumber on|off"; }
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
    auto to_lower = [](std::string s){ for (auto& c: s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; };
    std::string v = to_lower(args[0]);
    short col = -1;
    if (v == "default" || v == "normal") col = -1;
    else if (v == "black") col = COLOR_BLACK;
    else if (v == "white") col = COLOR_WHITE;
    else if (v == "green") col = COLOR_GREEN;
    else if (v == "yellow") col = COLOR_YELLOW;
    else if (v == "red") col = COLOR_RED;
    else if (v == "blue") col = COLOR_BLUE;
    else if (v == "magenta") col = COLOR_MAGENTA;
    else if (v == "cyan") col = COLOR_CYAN;
    else { message = "set background: unknown color"; return; }
    term.setBackground(col);
    message = std::string("background=") + v;
  });
  registry.register_command("set searchcolor", [this](const std::vector<std::string>& args){
    if (args.empty()) { message = "set searchcolor: use :set searchcolor default|black|white|red|green|blue|yellow|magenta|cyan"; return; }
    auto to_lower = [](std::string s){ for (auto& c: s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; };
    std::string v = to_lower(args[0]);
    short col = -1;
    if (v == "default" || v == "normal") col = -1;
    else if (v == "black") col = COLOR_BLACK;
    else if (v == "white") col = COLOR_WHITE;
    else if (v == "red") col = COLOR_RED;
    else if (v == "green") col = COLOR_GREEN;
    else if (v == "blue") col = COLOR_BLUE;
    else if (v == "yellow") col = COLOR_YELLOW;
    else if (v == "magenta") col = COLOR_MAGENTA;
    else if (v == "cyan") col = COLOR_CYAN;
    else { message = "set searchhl: unknown color"; return; }
    term.setSearchHighlight(col);
    message = std::string("searchhl=") + v;
  });
  registry.register_command("backend", [this](const std::vector<std::string>& args){
    (void)args;
    message = "backend=";
    message += buf.backend_name();
  });
  registry.register_command("set onemore", [this](const std::vector<std::string>& args){
    if (args.empty()){
      virtualedit_onemore = !virtualedit_onemore;
      message = virtualedit_onemore ? "onemore on" : "onemore off";
    } else {
      message = "set onemore: use :set onemore on|off";
      std::string opt = args[0];
      if (opt == "on") { virtualedit_onemore = true; message = "onemore on"; }
      else if (opt == "off") { virtualedit_onemore = false; message = "onemore off"; }
      else { message = "set onemore: use :set onemore on|off"; }
    }
  });
  registry.register_command("set mouse", [this](const std::vector<std::string>& args){
    if (args.empty()){
      enable_mouse = !enable_mouse;
      if (enable_mouse) {
        mmask_t mask = BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED;
        #ifdef BUTTON4_PRESSED
        mask |= BUTTON4_PRESSED;
        #endif
        #ifdef BUTTON5_PRESSED
        mask |= BUTTON5_PRESSED;
        #endif
        mouseinterval(0);
        mousemask(mask, nullptr);
        message = "mouse on";
      } else {
        mousemask(0, nullptr);
        mouseinterval(200);
        message = "mouse off";
      }
    }else{
      message = "set mouse: use :set mouse on|off";
      std::string opt = args[0];
      if (opt == "on") {
        enable_mouse = true;
        mmask_t mask = BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED;
        #ifdef BUTTON4_PRESSED
        mask |= BUTTON4_PRESSED;
        #endif
        #ifdef BUTTON5_PRESSED
        mask |= BUTTON5_PRESSED;
        #endif
        mouseinterval(0);
        mousemask(mask, nullptr);
        message = "mouse on";
      }
      else if (opt == "off") {
        enable_mouse = false;
        mousemask(0, nullptr);
        mouseinterval(200);
        message = "mouse off";
      }
      else { message = "set mouse: use :set mouse on|off"; }
    }
  });
  registry.register_command("set autoindent", [this](const std::vector<std::string>& args){
    if (args.empty()) {
      auto_indent = !auto_indent;
      message = auto_indent ? "autoindent on" : "autoindent off";
    } else {
      std::string opt = args[0];
      if (opt == "on") { auto_indent = true; message = "autoindent on"; }
      else if (opt == "off") { auto_indent = false; message = "autoindent off"; }
      else { message = "set autoindent: use :set autoindent on|off"; }
    }
  });
  registry.register_command("vsplit", [this](const std::vector<std::string>& args){
    std::optional<std::filesystem::path> p;
    if (!args.empty()) p = std::filesystem::path(args[0]);
    split_vertical(p);
  });
  registry.register_command("vsp", [this](const std::vector<std::string>& args){
    std::optional<std::filesystem::path> p;
    if (!args.empty()) p = std::filesystem::path(args[0]);
    split_vertical(p);
  });
  registry.register_command("hsplit", [this](const std::vector<std::string>& args){
    std::optional<std::filesystem::path> p;
    if (!args.empty()) p = std::filesystem::path(args[0]);
    split_horizontal(p);
  });
  registry.register_command("split", [this](const std::vector<std::string>& args){
    std::optional<std::filesystem::path> p;
    if (!args.empty()) p = std::filesystem::path(args[0]);
    split_horizontal(p);
  });
  registry.register_command("sp", [this](const std::vector<std::string>& args){
    std::optional<std::filesystem::path> p;
    if (!args.empty()) p = std::filesystem::path(args[0]);
    split_horizontal(p);
  });
  registry.register_command("close", [this](const std::vector<std::string>&){
    if (!close_active_pane()) message = "cannot close last pane";
  });
  registry.register_command("focus", [this](const std::vector<std::string>& args){
    if (args.empty()) { message = "focus <index>"; return; }
    try {
      int idx = std::stoi(args[0]) - 1;
      set_active_pane(idx);
    } catch (...) {
      message = "focus <index>";
    }
  });
  registry.register_command("edit", [this](const std::vector<std::string>& args){
    if (args.empty()) { message = "edit <file> [more files]"; return; }
    open_path_in_pane(active_pane, args[0]);
    for (size_t i = 1; i < args.size(); ++i) {
      std::filesystem::path p = args[i];
      split_vertical(p);
    }
  });
}

#undef buf
#undef modified
#undef file_path
