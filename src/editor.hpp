#pragma once
#include <optional>
#include <filesystem>
#include <string>
#include <vector>
#include "types.hpp"
#include "text_buffer.hpp"
#include "input.hpp"
#include "undo_manager.hpp"
#include "renderer.hpp"
#include "ncurses_terminal.hpp"
#include "cmd_registry.hpp"

class Editor {
public:
  explicit Editor(const std::optional<std::filesystem::path>& file);
  void run();

private:
  struct Register {
    std::vector<std::string> lines;
    bool linewise = true;
  } reg;

  TextBuffer buf;
  Cursor cur;
  Viewport vp;
  Mode mode = Mode::Normal;
  int tab_width = 4;
  bool show_line_numbers = false;
  bool enable_color = true;
  bool enable_mouse = false;
  UndoManager um;
  bool modified = false;
  bool should_quit = false;
  std::optional<std::filesystem::path> file_path;
  std::string message;
  std::string cmdline;
  Input input;
  Renderer renderer;
  NcursesTerminal term;
  CommandRegistry registry;
  Cursor visual_anchor{0,0};
  bool visual_active = false;
  bool last_search_forward = true;
  std::string last_search;
  bool auto_pair = false;
  enum class PendingOp { None, Delete, Yank };
  PendingOp pending_op = PendingOp::None;

  void render();
  void handle_input(int ch);
  void handle_normal_input(int ch);
  void handle_insert_input(int ch);
  void handle_command_input(int ch);
  void handle_mouse();
  void execute_command();
  void register_commands();
  void move_left();
  void move_right();
  void move_up();
  void move_down();
  void delete_char();
  void delete_line();
  void delete_lines_range(int start_row, int count);
  void reg_push_line(int row);
  void paste_below();
  void insert_line_below();
  void insert_line_above();
  void split_line_at_cursor();
  void backspace();
  void undo();
  void redo();
  void begin_group();
  void commit_group();
  void push_op(const Operation& op);
  void search_forward(const std::string& pattern);
  void search_backward(const std::string& pattern);
  void repeat_last_search(bool is_forward);
  void delete_to_next_word();
  void yank_to_next_word();
  void delete_to_word_end();
  void yank_to_word_end();
  void move_to_top();
  void move_to_bottom();
  void move_to_next_word_left();
  void move_to_next_word_right();
  void move_to_previous_word_left();
  void move_to_beginning_of_line();
  void move_to_end_of_line();
  void scroll_up_half_page();
  void scroll_down_half_page();
  void enter_visual_char();
  void enter_visual_line();
  void exit_visual();
  void get_visual_range(int& r0, int& r1, int& c0, int& c1) const;
  void delete_selection();
  void yank_selection();
  void set_tab_width(int width);
  void insert_pair(int ch);
  void load_rc();
};
