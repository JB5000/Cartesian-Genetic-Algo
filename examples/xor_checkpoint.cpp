#include <cstdlib>
#include <iostream>
#include <string>

#include "cga/checkpoint.hpp"
#include "cga/dataset.hpp"
#include "cga/evolution.hpp"
#include "cga/metrics.hpp"

int main(int argc, char** argv) {
  std::size_t generations = 250;
  std::uint32_t seed = 42;
  std::string checkpoint_path = "xor.ckpt";
  for (int index = 1; index + 1 < argc; ++index) {
    const std::string option = argv[index];
    if (option == "--generations") generations = std::stoul(argv[++index]);
    else if (option == "--seed") seed = static_cast<std::uint32_t>(std::stoul(argv[++index]));
    else if (option == "--checkpoint") checkpoint_path = argv[++index];
  }

  const cga::Dataset data{
      {{0.0F, 0.0F}, {0.0F}}, {{0.0F, 1.0F}, {1.0F}},
      {{1.0F, 0.0F}, {1.0F}}, {{1.0F, 1.0F}, {0.0F}}};
  cga::EvolutionConfig config;
  config.population_size = 64;
  config.generations = generations;
  config.mutation.node_rate = 0.12;
  config.mutation.output_rate = 0.08;
  config.seed = seed;

  const auto result = cga::evolve(
      2, 16, 1,
      [&](const cga::Genome& genome) { return cga::mean_squared_error(genome, data); },
      config);
  cga::save_checkpoint(checkpoint_path,
                       {result.best, result.best_fitness, result.evaluations});
  std::cout << "saved=" << checkpoint_path << " fitness=" << result.best_fitness
            << " accuracy=" << cga::binary_accuracy(result.best, data) << '\n';
}
