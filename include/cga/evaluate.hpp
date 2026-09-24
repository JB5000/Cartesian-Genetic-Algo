#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "cga/genome.hpp"

namespace cga {

inline float apply(OpCode op, float left, float right) noexcept {
  switch (op) {
    case OpCode::Add: return left + right;
    case OpCode::Subtract: return left - right;
    case OpCode::Multiply: return left * right;
    case OpCode::Maximum: return std::max(left, right);
    case OpCode::Minimum: return std::min(left, right);
    case OpCode::Tanh: return std::tanh(left);
    case OpCode::Sine: return std::sin(left);
    case OpCode::Identity: return left;
  }
  return 0.0F;
}

struct Evaluation {
  std::vector<float> outputs;
  std::size_t active_nodes = 0;
};

inline Evaluation evaluate(const Genome& genome, const std::vector<float>& inputs) {
  genome.require_valid();
  if (inputs.size() != genome.input_count) {
    throw std::invalid_argument("input count does not match genome");
  }

  std::vector<float> values = inputs;
  values.reserve(genome.value_count());
  for (const Node& node : genome.nodes) {
    const float left = values[static_cast<std::size_t>(node.inputs[0])];
    const float right = values[static_cast<std::size_t>(node.inputs[1])];
    values.push_back(apply(node.op, left, right));
  }

  Evaluation result;
  result.active_nodes = genome.active_nodes().size();
  result.outputs.reserve(genome.outputs.size());
  for (const int output : genome.outputs) {
    result.outputs.push_back(values[static_cast<std::size_t>(output)]);
  }
  return result;
}

}  // namespace cga
