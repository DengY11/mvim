#pragma once
/*
 * Types
 *
 * Purpose: shared lightweight structs/enums (Mode/Cursor/Viewport).
 * Principle: carry simple state; avoid cross-module coupling/business logic.
 */
#include <filesystem>

enum class Mode { Normal, Insert, Command, Visual, VisualLine };

struct Cursor { int row = 0; int col = 0; };
struct Viewport { int top_line = 0; int left_col = 0; };
