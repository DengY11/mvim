#pragma once
/*
 * CommandRegistry
 *
 * Purpose: register and dispatch Ex commands.
 * Design: map name → handler (args vector); Editor parses and routes.
 */
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

class CommandRegistry {
public:
  using Handler = std::function<void(const std::vector<std::string>&)>;
  void register_command(const std::string& name, Handler h) { map_[name] = std::move(h); }
  bool execute(const std::string& name, const std::vector<std::string>& args) const {
    auto it = map_.find(name);
    if (it == map_.end()) return false;
    it->second(args);
    return true;
  }
private:
  std::unordered_map<std::string, Handler> map_;
};
