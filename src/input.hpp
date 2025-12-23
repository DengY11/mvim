#pragma once
#include <cstddef>
/*
 * Input
 *
 * Purpose: parse Normal mode double key prefixes (dd/yy) with minimal state.
 * Extend: supports count prefixes; decoupled from concrete editing actions.
 */

class Input {
public:
  bool consumeDd(int ch);
  bool consumeYy(int ch);
  bool consumeGg(int ch);
  bool consumeGt(int ch);
  bool consumeLt(int ch);
  bool consumeDigit(int ch);
  bool hasCount() const;
  size_t takeCount();
  void reset();
private:
  bool pending_d_ = false;
  bool pending_y_ = false;
  bool pending_g_ = false;
  bool pending_gt_ = false;
  bool pending_lt_ = false;
  size_t pending_count_ = 0;
};
