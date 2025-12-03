#pragma once
#include <vector>
#include <string>
#include "types.hpp"
#include "text_buffer.hpp"

struct Operation {
  enum Type { InsertChar, DeleteChar, InsertLine, DeleteLine, ReplaceLine, InsertLinesBlock, DeleteLinesBlock } type;
  int row;
  int col;
  std::string payload;
  std::string alt_payload;
};

struct UndoEntry {
  std::vector<Operation> ops;
  Cursor pre;
  Cursor post;
};

class UndoManager {
public:
  void begin_group(const Cursor& pre);
  void push_op(const Operation& op);
  void commit_group(const Cursor& post);
  void clear_redo();
  bool can_undo() const;
  bool can_redo() const;
  void undo(TextBuffer& buf, Cursor& cur);
  void redo(TextBuffer& buf, Cursor& cur);

private:
  std::vector<UndoEntry> undo_entries_;
  std::vector<UndoEntry> redo_entries_;
  bool grouping_ = false;
  UndoEntry current_;
};

