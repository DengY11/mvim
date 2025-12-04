#pragma once
#include <unistd.h>

class UniqueFd {
public:
  UniqueFd() : fd_(-1) {}
  explicit UniqueFd(int fd) : fd_(fd) {}
  UniqueFd(const UniqueFd&) = delete;
  UniqueFd& operator=(const UniqueFd&) = delete;
  UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
  UniqueFd& operator=(UniqueFd&& other) noexcept {
    if (this != &other) { close_if_needed(); fd_ = other.fd_; other.fd_ = -1; }
    return *this;
  }
  ~UniqueFd() { close_if_needed(); }
  int get() const { return fd_; }
  bool valid() const { return fd_ >= 0; }
  void reset(int fd = -1) { close_if_needed(); fd_ = fd; }
private:
  void close_if_needed() { if (fd_ >= 0) { ::close(fd_); fd_ = -1; } }
  int fd_;
};
