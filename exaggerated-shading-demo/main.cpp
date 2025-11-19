#include "viewer.hpp"

int main(int argc, char* argv[]) {
  demo::viewer viewer{};
  for (int i = 1; i < argc; ++i) viewer.eval_lua_file(argv[i]);
  viewer.run();
}
