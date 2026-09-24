#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace cga {

enum class OpCode : std::uint8_t {
  Add,
  Subtract,
  Multiply,
  Maximum,
  Minimum,
  Tanh,
  Sine,
  Identity,
};

struct Node {
  OpCode op = OpCode::Add;
  std::array<int, 2> inputs{0, 0};
};

struct Genome {
  std::size_t input_count = 0;
  std::vector<Node> nodes;
  std::vector<int> outputs;

  std::size_t value_count() const noexcept { return input_count + nodes.size(); }

  bool valid() const noexcept {
    for (std::size_t index = 0; index < nodes.size(); ++index) {
      const int limit = static_cast<int>(input_count + index);
      for (const int input : nodes[index].inputs) {
        if (input < 0 || input >= limit) return false;
      }
    }
    for (const int output : outputs) {
      if (output < 0 || output >= static_cast<int>(value_count())) return false;
    }
    return true;
  }

  void require_valid() const {
    if (!valid()) throw std::invalid_argument("invalid CGP genome");
  }

  std::vector<int> active_nodes() const {
    std::vector<bool> seen(nodes.size(), false);
    std::vector<int> active;
    const auto visit = [&](auto&& self, int value) -> void {
      if (value < static_cast<int>(input_count)) return;
      const std::size_t node_index = static_cast<std::size_t>(value - input_count);
      if (seen[node_index]) return;
      seen[node_index] = true;
      for (const int input : nodes[node_index].inputs) self(self, input);
      active.push_back(static_cast<int>(node_index));
    };
    for (const int output : outputs) visit(visit, output);
    return active;
  }
};

}  // namespace cga
