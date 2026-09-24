#pragma once

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include "cga/evaluate.hpp"

namespace cga {

struct Sample {
  std::vector<float> inputs;
  std::vector<float> targets;
};

using Dataset = std::vector<Sample>;

inline float mean_squared_error(const Genome& genome, const Dataset& data) {
  if (data.empty()) return std::numeric_limits<float>::infinity();
  double total = 0.0;
  std::size_t count = 0;
  for (const Sample& sample : data) {
    const auto result = evaluate(genome, sample.inputs);
    if (result.outputs.size() != sample.targets.size()) {
      throw std::invalid_argument("target count does not match genome outputs");
    }
    for (std::size_t index = 0; index < result.outputs.size(); ++index) {
      const double error = static_cast<double>(result.outputs[index]) - sample.targets[index];
      total += error * error;
      ++count;
    }
  }
  return static_cast<float>(total / static_cast<double>(count));
}

}  // namespace cga
