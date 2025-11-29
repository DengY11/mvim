#pragma once
#include <cstddef>
/*
 * Input
 *
 * 作用：解析 Normal 模式下的双击键序前缀（dd/yy），提供最小状态机。
 * 扩展：预留计数前缀与更通用的键序解析，不耦合具体编辑动作。
 */

class Input {
public:
  bool consumeDd(int ch);
  bool consumeYy(int ch);
  bool consumeGg(int ch);
  bool consumeDigit(int ch);
  bool hasCount() const;
  size_t takeCount();
  void reset();
private:
  bool pending_d_ = false;
  bool pending_y_ = false;
  bool pending_g_ = false;
  size_t pending_count_ = 0;
};
