#pragma once

namespace demo {

template <typename... params>
struct basic_update_list {
  using update_type = std::function<void(params...)>;
  using list_type = std::list<update_type>;

  constexpr void push(auto&& f) {
    updates.push_back(std::forward<decltype(f)>(f));
  }

  constexpr void run(params&&... args) {
    for (auto& update : updates) update(std::forward<params>(args)...);
  }

 private:
  list_type updates{};
};

}  // namespace demo
