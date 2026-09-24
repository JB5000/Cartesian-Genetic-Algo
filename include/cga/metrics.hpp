#pragma once

#include <cmath>
#include <limits>

#include "cga/dataset.hpp"

namespace cga {

inline float mean_absolute_error(const Genome& genome, const Dataset& data) {
  if (data.empty()) return std::numeric_limits<float>::infinity();
  double total = 0.0;
  std::size_t count = 0;
  for (const Sample& sample : data) {
    const auto result = evaluate(genome, sample.inputs);
    if (result.outputs.size() != sample.targets.size()) {
      throw std::invalid_argument("target count does not match genome outputs");
    }
    for (std::size_t index = 0; index < result.outputs.size(); ++index) {
      total += std::abs(static_cast<double>(result.outputs[index]) - sample.targets[index]);
      ++count;
    }
  }
  return static_cast<float>(total / static_cast<double>(count));
}

inline float binary_accuracy(const Genome& genome, const Dataset& data,
                             float threshold = 0.5F) {
  if (data.empty()) return 0.0F;
  std::size_t correct = 0;
  for (const Sample& sample : data) {
    const auto result = evaluate(genome, sample.inputs);
    if (result.outputs.size() != 1 || sample.targets.size() != 1) {
      throw std::invalid_argument("binary accuracy requires one output");
    }
    const bool predicted = result.outputs.front() >= threshold;
    const bool expected = sample.targets.front() >= threshold;
    correct += predicted == expected;
  }
  return static_cast<float>(correct) / static_cast<float>(data.size());
}

inline float active_node_ratio(const Genome& genome) {
  if (genome.nodes.empty()) return 0.0F;
  return static_cast<float>(genome.active_nodes().size()) /
         static_cast<float>(genome.nodes.size());
}

}  // namespace cga
