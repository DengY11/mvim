#pragma once
/*
 * Terminal
 *
 * 作用：封装 ncurses 的初始化与还原，提供 RAII 生命周期管理。
 * 用法：在 main 中构造实例，程序退出时自动 endwin() 还原终端。
 * 依赖：ncurses；不负责渲染逻辑，仅管理终端模式（raw/noecho/keypad）。
 */
#include <ncurses.h>

class Terminal {
public:
  Terminal();
  ~Terminal();
};
