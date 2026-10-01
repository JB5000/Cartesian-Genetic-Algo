#include <iostream>
#include <random>

#include "cga/cga.hpp"

int main() {
  const cga::Dataset xor_data{{{0.0F, 0.0F}, {0.0F}},
                              {{0.0F, 1.0F}, {1.0F}},
                              {{1.0F, 0.0F}, {1.0F}},
                              {{1.0F, 1.0F}, {0.0F}}};
  std::mt19937 rng(20261001);
  cga::Genome genome = cga::random_genome(2, 12, 1, rng);
  const float initial = cga::semantic_distance(genome, xor_data);

  cga::SemanticMutationConfig config;
  config.trials = 8;
  config.mutation.node_rate = 0.20;
  config.mutation.output_rate = 0.10;
  for (int iteration = 0; iteration < 500; ++iteration) {
    cga::semantic_mutate(genome, xor_data, rng, config);
  }

  const float final = cga::semantic_distance(genome, xor_data);
  std::cout << "semantic_xor initial_mse=" << initial
            << " final_mse=" << final
            << " active_nodes=" << genome.active_nodes().size() << '\n';
  return genome.valid() ? 0 : 1;
}
