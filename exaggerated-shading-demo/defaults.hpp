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

}  // namespace demo
