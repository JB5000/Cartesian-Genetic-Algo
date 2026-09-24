#include <iomanip>
#include <iostream>

#include "cga/dataset.hpp"
#include "cga/evolution.hpp"
#include "cga/metrics.hpp"

int main() {
  const cga::Dataset xor_data{
      {{0.0F, 0.0F}, {0.0F}},
      {{0.0F, 1.0F}, {1.0F}},
      {{1.0F, 0.0F}, {1.0F}},
      {{1.0F, 1.0F}, {0.0F}},
  };

  cga::EvolutionConfig config;
  config.population_size = 64;
  config.generations = 250;
  config.mutation.node_rate = 0.12;
  config.mutation.output_rate = 0.08;
  config.seed = 42;

  const auto result = cga::evolve(
      2, 16, 1,
      [&](const cga::Genome& genome) {
        return cga::mean_squared_error(genome, xor_data);
      },
      config);

  std::cout << std::fixed << std::setprecision(6)
            << "best_mse=" << result.best_fitness
            << " evaluations=" << result.evaluations
            << " active_nodes=" << result.best.active_nodes().size()
            << " accuracy=" << cga::binary_accuracy(result.best, xor_data) << '\n';
  for (const auto& sample : xor_data) {
    const auto prediction = cga::evaluate(result.best, sample.inputs).outputs.front();
    std::cout << sample.inputs[0] << " xor " << sample.inputs[1]
              << " -> " << prediction << '\n';
  }
}
