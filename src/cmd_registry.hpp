#pragma once
/*
 * CommandRegistry
 *
 * 作用：命令注册与分发中心，统一管理 Ex 命令执行入口。
 * 设计：名称到处理器的映射，处理器接受参数向量；Editor 负责解析与路由。
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
