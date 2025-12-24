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
#include "pane_layout.hpp"
#include <memory>
#include <unordered_map>

class Editor {
public:
  explicit Editor(const std::optional<std::filesystem::path>& file);
  void run();

private:
  struct Document {
    TextBuffer buf;
    UndoManager um;
    std::optional<UndoEntry> last_change;
    std::optional<std::filesystem::path> file_path;
    bool modified = false;
  };

  struct Pane {
    std::shared_ptr<Document> doc;
    Cursor cur;
    Viewport vp;
  };

  Pane& pane();
  const Pane& pane() const;
  Document& doc();
  const Document& doc() const;
  void set_active_pane(int idx);
  int create_pane_from_file(const std::optional<std::filesystem::path>& file);
  void split_vertical(const std::optional<std::filesystem::path>& file);
  void split_horizontal(const std::optional<std::filesystem::path>& file);
  bool close_active_pane();
  bool close_or_quit(bool force);
  int active_pane_count() const;
  void open_path_in_pane(int idx, const std::filesystem::path& path);
  void collect_layout(std::vector<PaneRect>& out) const;
  void focus_next_pane();
  void focus_direction(char dir);

  struct Register {
    std::vector<std::string> lines;
    bool linewise = true;
  } reg;

  Mode mode = Mode::Normal;
  int tab_width = 4;
  bool show_line_numbers = false;
  bool relative_line_numbers = false;
  bool enable_color = false;
  bool auto_indent = false;
  bool enable_mouse = false;
  bool should_quit = false;
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
  std::vector<SearchHit> last_search_hits;
  bool virtualedit_onemore = false;
  enum class PendingOp { None, Delete, Yank };
  PendingOp pending_op = PendingOp::None;
  bool insert_buffer_active = false;
  int insert_buffer_row = -1;
  std::string insert_buffer_line;
  std::vector<Pane> panes;
  int active_pane = 0;
  std::unique_ptr<SplitNode> layout;
  std::unordered_map<std::string, std::weak_ptr<Document>> doc_table;
  bool pending_ctrl_w = false;

  void render();
  void handle_input(int ch);
  void handle_normal_input(int ch);
  void handle_insert_input(int ch);
  void handle_command_input(int ch);
  void handle_mouse();
  void begin_insert_buffer();
  void apply_insert_char(int ch);
  void apply_backspace();
  void commit_insert_buffer();
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
  void recompute_search_hits(const std::string& pattern);
  int max_col_for_row(int row) const;
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
  void repeat_last_change();
  void apply_operation_forward(const Operation& op, int row_delta, int col_delta);
  void indent_lines(int start_row, int count);
  void dedent_lines(int start_row, int count);
  std::string compute_indent_for_line(int row) const;
  void apply_indent_to_newline(const std::string& indent);
};
