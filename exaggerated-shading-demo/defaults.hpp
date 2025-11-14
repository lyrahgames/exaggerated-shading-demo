#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <numbers>
#include <print>
#include <stdexcept>
#include <vector>
//
#include "opengl/opengl.hpp"

namespace demo {

using namespace std;
using namespace gl;

using czstring = const char*;
using zstring = char*;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
//
using uint = unsigned int;

using float32 = float;
using float64 = double;
//
using real = float32;  // default float type

using glm::ivec2;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

inline constexpr auto pi = numbers::pi_v<real>;
inline constexpr auto infinity = numeric_limits<real>::infinity();

template <typename... types>
void breakpoint() = delete;
template <typename... types>
void breakpoint(types...) = delete;

// scoped_chdir -> scoped_current_path
// this is not shared_scoped_current_path
// and as such not save in a multi-threaded context
// `std::filesystem::current_path` itself is atomic
// but also not save for multiple threads due to the shared global state.
struct scoped_chdir final {
  std::filesystem::path cwd{};
  scoped_chdir(std::filesystem::path const& path)
      : cwd{std::filesystem::current_path()} {
    current_path(path);
  }
  ~scoped_chdir() noexcept { current_path(cwd); }
};

/// Return the content of a file given by its path as a standard `string` object.
///
inline auto string_from_file(std::filesystem::path const& path) -> std::string {
  // We will read all characters as block and open the file in binary mode.
  // Make sure to jump to its end for directly reading its size.
  std::ifstream file{path, std::ios::binary | std::ios::ate};
  if (!file)
    throw std::runtime_error(
        std::format("Failed to open file '{}'.", path.string()));
  // Read the file's size.
  const auto size = file.tellg();
  // Prepare the result string with a sufficiently large buffer.
  string result(size, '\0');
  // Go back to the start and read all characters at once in a block.
  file.seekg(0);
  file.read(result.data(), size);
  return result;
};

}  // namespace demo
