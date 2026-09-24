#pragma once

#include <algorithm>
#include <random>

#include "cga/genome.hpp"

namespace cga {

inline OpCode random_opcode(std::mt19937& rng) {
  std::uniform_int_distribution<int> choice(0, 7);
  return static_cast<OpCode>(choice(rng));
}

inline Genome random_genome(std::size_t inputs, std::size_t nodes,
                            std::size_t outputs, std::mt19937& rng) {
  Genome genome;
  genome.input_count = inputs;
  genome.nodes.resize(nodes);
  genome.outputs.resize(outputs);
  for (std::size_t index = 0; index < nodes; ++index) {
    std::uniform_int_distribution<int> source(0, static_cast<int>(inputs + index - 1));
    genome.nodes[index] = {random_opcode(rng), {source(rng), source(rng)}};
  }
  std::uniform_int_distribution<int> output_source(
      0, static_cast<int>(genome.value_count() - 1));
  for (int& output : genome.outputs) output = output_source(rng);
  return genome;
}

struct MutationConfig {
  double node_rate = 0.05;
  double output_rate = 0.05;
};

inline std::size_t mutate(Genome& genome, std::mt19937& rng,
                          MutationConfig config = {}) {
  std::uniform_real_distribution<double> chance(0.0, 1.0);
  std::size_t changes = 0;
  for (std::size_t index = 0; index < genome.nodes.size(); ++index) {
    Node& node = genome.nodes[index];
    const int limit = static_cast<int>(genome.input_count + index - 1);
    std::uniform_int_distribution<int> source(0, limit);
    if (chance(rng) < config.node_rate) {
      node.op = random_opcode(rng);
      ++changes;
    }
    for (int& input : node.inputs) {
      if (chance(rng) < config.node_rate) {
        input = source(rng);
        ++changes;
      }
    }
  }
  std::uniform_int_distribution<int> output_source(
      0, static_cast<int>(genome.value_count() - 1));
  for (int& output : genome.outputs) {
    if (chance(rng) < config.output_rate) {
      output = output_source(rng);
      ++changes;
    }
  }
  return changes;
}

}  // namespace cga
