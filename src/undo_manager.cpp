#include "undo_manager.hpp"

void UndoManager::beginGroup(const Cursor& pre) {
  if (!grouping_) {
    grouping_ = true;
    current_.ops.clear();
    current_.pre = pre;
  }
}

void UndoManager::pushOp(const Operation& op) {
  if (grouping_) {
    current_.ops.push_back(op);
  }
}

void UndoManager::commitGroup(const Cursor& post) {
  if (grouping_) {
    grouping_ = false;
    current_.post = post;
    if (!current_.ops.empty()) {
      undo_entries_.push_back(current_);
      redo_entries_.clear();
    }
    current_.ops.clear();
  }
}

void UndoManager::clear_redo() { redo_entries_.clear(); }
bool UndoManager::canUndo() const { return !undo_entries_.empty(); }
bool UndoManager::canRedo() const { return !redo_entries_.empty(); }

void UndoManager::undo(TextBuffer& buf, Cursor& cur) {
  if (undo_entries_.empty()) return;
  UndoEntry e = undo_entries_.back();
  undo_entries_.pop_back();
  for (int i = (int)e.ops.size() - 1; i >= 0; --i) {
    const Operation& op = e.ops[i];
    switch (op.type) {
      case Operation::InsertChar: {
        std::string s = buf.line(op.row);
        if (op.col < (int)s.size()) {
          s.erase(s.begin() + op.col);
          buf.replace_line(op.row, s);
        }
      } break;
      case Operation::DeleteChar: {
        std::string s = buf.line(op.row);
        if (op.col <= (int)s.size()) {
          s.insert(s.begin() + op.col, op.payload[0]);
          buf.replace_line(op.row, s);
        }
      } break;
      case Operation::InsertLine: {
        if (op.row < buf.line_count()) buf.erase_line(op.row);
      } break;
      case Operation::DeleteLine: {
        buf.insert_line(op.row, op.payload);
      } break;
      case Operation::ReplaceLine: {
        if (op.row >= 0 && op.row < buf.line_count()) buf.replace_line(op.row, op.payload);
      } break;
      case Operation::InsertLinesBlock: {
        int start = op.row;
        int count = 0;
        for (size_t p = 0; p <= op.payload.size(); ++p) if (p == op.payload.size() || op.payload[p] == '\n') count++;
        if (start >= 0 && start + count <= buf.line_count()) buf.erase_lines(start, start + count);
      } break;
      case Operation::DeleteLinesBlock: {
        int start = op.row;
        std::vector<std::string> lines; lines.reserve(16);
        size_t st = 0;
        while (st <= op.payload.size()) {
          size_t pos = op.payload.find('\n', st);
          if (pos == std::string::npos) { lines.emplace_back(op.payload.substr(st)); break; }
          lines.emplace_back(op.payload.substr(st, pos - st));
          st = pos + 1;
        }
        buf.insert_lines(start, lines);
      } break;
    }
  }
  cur = e.pre;
  redo_entries_.push_back(e);
}

void UndoManager::redo(TextBuffer& buf, Cursor& cur) {
  if (redo_entries_.empty()) return;
  UndoEntry e = redo_entries_.back();
  redo_entries_.pop_back();
  for (size_t i = 0; i < e.ops.size(); ++i) {
    const Operation& op = e.ops[i];
    switch (op.type) {
      case Operation::InsertChar: {
        std::string s = buf.line(op.row);
        if (op.col <= (int)s.size()) {
          s.insert(s.begin() + op.col, op.payload[0]);
          buf.replace_line(op.row, s);
        }
      } break;
      case Operation::DeleteChar: {
        std::string s = buf.line(op.row);
        if (op.col < (int)s.size()) {
          s.erase(s.begin() + op.col);
          buf.replace_line(op.row, s);
        }
      } break;
      case Operation::InsertLine: {
        buf.insert_line(op.row, op.payload);
      } break;
      case Operation::DeleteLine: {
        if (op.row < buf.line_count()) buf.erase_line(op.row);
      } break;
      case Operation::ReplaceLine: {
        if (op.row >= 0 && op.row < buf.line_count()) buf.replace_line(op.row, op.alt_payload);
      } break;
      case Operation::InsertLinesBlock: {
        int start = op.row;
        std::vector<std::string> lines; lines.reserve(16);
        size_t st = 0;
        while (st <= op.payload.size()) {
          size_t pos = op.payload.find('\n', st);
          if (pos == std::string::npos) { lines.emplace_back(op.payload.substr(st)); break; }
          lines.emplace_back(op.payload.substr(st, pos - st));
          st = pos + 1;
        }
        buf.insert_lines(start, lines);
      } break;
      case Operation::DeleteLinesBlock: {
        int start = op.row;
        int count = 0;
        for (size_t p = 0; p <= op.payload.size(); ++p) if (p == op.payload.size() || op.payload[p] == '\n') count++;
        if (start >= 0 && start + count <= buf.line_count()) buf.erase_lines(start, start + count);
      } break;
    }
  }
  cur = e.post;
  undo_entries_.push_back(e);
}
