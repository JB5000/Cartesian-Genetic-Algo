#pragma once

#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>

#include "cga/evolution.hpp"

namespace cga {

struct Checkpoint {
  Genome genome;
  float fitness = 0.0F;
  std::size_t evaluations = 0;
};

inline void save_checkpoint(const std::string& path, const Checkpoint& checkpoint) {
  checkpoint.genome.require_valid();
  std::ofstream output(path);
  if (!output) throw std::runtime_error("cannot open checkpoint for writing: " + path);
  output << "CGA_CHECKPOINT 1\n";
  output << checkpoint.fitness << ' ' << checkpoint.evaluations << '\n';
  output << checkpoint.genome.input_count << ' ' << checkpoint.genome.nodes.size()
         << ' ' << checkpoint.genome.outputs.size() << '\n';
  for (const Node& node : checkpoint.genome.nodes) {
    output << static_cast<int>(node.op) << ' ' << node.inputs[0] << ' '
           << node.inputs[1] << '\n';
  }
  for (const int output_index : checkpoint.genome.outputs) output << output_index << ' ';
  output << '\n';
}

inline Checkpoint load_checkpoint(const std::string& path) {
  std::ifstream input(path);
  if (!input) throw std::runtime_error("cannot open checkpoint for reading: " + path);
  std::string magic;
  int version = 0;
  if (!(input >> magic >> version) || magic != "CGA_CHECKPOINT" || version != 1) {
    throw std::runtime_error("unsupported checkpoint format");
  }
  Checkpoint checkpoint;
  if (!(input >> checkpoint.fitness >> checkpoint.evaluations)) {
    throw std::runtime_error("invalid checkpoint header");
  }
  std::size_t nodes = 0;
  std::size_t outputs = 0;
  if (!(input >> checkpoint.genome.input_count >> nodes >> outputs)) {
    throw std::runtime_error("invalid checkpoint shape");
  }
  checkpoint.genome.nodes.resize(nodes);
  checkpoint.genome.outputs.resize(outputs);
  for (Node& node : checkpoint.genome.nodes) {
    int op = 0;
    if (!(input >> op >> node.inputs[0] >> node.inputs[1]) || op < 0 || op > 7) {
      throw std::runtime_error("invalid checkpoint node");
    }
    node.op = static_cast<OpCode>(op);
  }
  for (int& output : checkpoint.genome.outputs) {
    if (!(input >> output)) throw std::runtime_error("invalid checkpoint output");
  }
  checkpoint.genome.require_valid();
  return checkpoint;
}

}  // namespace cga
