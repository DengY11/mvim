#include "terminal.hpp"
#include "editor.hpp"
#include <optional>
#include <filesystem>

 
int main(int argc, char** argv) {
  std::optional<std::filesystem::path> path;
  if (argc >= 2) path = std::filesystem::path(argv[1]);
  Terminal term;
  Editor ed(path);
  ed.run();
  return 0;
}
