#pragma once
/*
 * Types
 *
 * 作用：共享的轻量数据结构与枚举（Mode/Cursor/Viewport）。
 * 原则：仅承载简单状态，避免跨模块强耦合与业务逻辑。
 */
#include <filesystem>

enum class Mode { Normal, Insert, Command, Visual, VisualLine };

struct Cursor { int row = 0; int col = 0; };
struct Viewport { int top_line = 0; };
