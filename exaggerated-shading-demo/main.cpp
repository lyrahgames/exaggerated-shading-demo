#include "viewer.hpp"

int main(int argc, char* argv[]) {
  demo::viewer viewer{};

  if (argc > 1) viewer.load_stl_surface(argv[1]);

  viewer.run();
}
