#include "input.hpp"

bool Input::consumeDd(int ch) {
  if (ch == 'd') {
    if (pending_d_) { pending_d_ = false; return true; }
    pending_d_ = true; return false;
  }
  return false;
}

bool Input::consumeYy(int ch) {
  if (ch == 'y') {
    if (pending_y_) { pending_y_ = false; return true; }
    pending_y_ = true; return false;
  }
  return false;
}

bool Input::consumeGg(int ch) {
  if (ch == 'g') {
    if (pending_g_) { pending_g_ = false; return true; }
    pending_g_ = true; return false;
  }
  return false;
}

bool Input::consumeGt(int ch) {
  if (ch == '>') {
    if (pending_gt_) { pending_gt_ = false; return true; }
    pending_gt_ = true; return false;
  }
  return false;
}

bool Input::consumeLt(int ch) {
  if (ch == '<') {
    if (pending_lt_) { pending_lt_ = false; return true; }
    pending_lt_ = true; return false;
  }
  return false;
}

bool Input::consumeDigit(int ch) {
  if (ch >= '1' && ch <= '9') {
    pending_count_ = pending_count_ * 10 + static_cast<size_t>(ch - '0');
    return true;
  }
  if (ch == '0') {
    if (pending_count_ > 0) {
      pending_count_ = pending_count_ * 10;
      return true;
    }
  }
  return false;
}

bool Input::hasCount() const {
  return pending_count_ > 0;
}

size_t Input::takeCount() {
  size_t c = pending_count_;
  pending_count_ = 0;
  return c;
}

void Input::reset() {
  pending_d_ = false;
  pending_y_ = false;
  pending_g_ = false;
  pending_gt_ = false;
  pending_lt_ = false;
}
