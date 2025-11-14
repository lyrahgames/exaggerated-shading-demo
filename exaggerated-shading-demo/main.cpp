#include "viewer.hpp"

int main(int argc, char* argv[]) {
  demo::viewer viewer{};

  if (argc > 1) viewer.load_scene(argv[1]);

  viewer.add_path(std::filesystem::current_path());

  viewer.run();
}
