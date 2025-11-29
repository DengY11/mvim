#pragma once
/*
 * HeadlessTerminal
 *
 * 作用：无头终端占位实现（待接入 ITerminal），用于自动化测试与渲染验证。
 * 状态：当前为空实现，不参与构建；未来实现绘制记录与断言能力。
 */
class HeadlessTerminal {
public:
  HeadlessTerminal();
  ~HeadlessTerminal();
};
