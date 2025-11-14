#pragma once
#include "defaults.hpp"

namespace demo::fdm {

using address = std::filesystem::path;

constexpr auto message_address(address addr) {
  return addr += ".fdm";
}

constexpr auto send_address(address addr) {
  return addr += ".send";
}

constexpr auto recv_address(address addr) {
  return addr += ".recv";
}

inline void send(address const& domain, std::string_view msg) {
  const auto msg_addr = message_address(domain);
  const auto tmp_addr = send_address(msg_addr);
  {
    std::ofstream file{tmp_addr, std::ios::binary};
    file << msg;
  }
  rename(tmp_addr, msg_addr);
}

inline auto recv(address const& domain) -> std::optional<std::string> {
  const auto msg_addr = message_address(domain);
  const auto tmp_addr = recv_address(msg_addr);

  std::error_code ec;
  rename(msg_addr, tmp_addr, ec);
  if (ec) return {};

  auto msg = string_from_file(tmp_addr);
  remove(tmp_addr);
  return std::move(msg);  // Move because NRVO cannot be applied.
}

}  // namespace demo::fdm
