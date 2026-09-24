#include <cassert>

#include "cga/evolution.hpp"

int main() {
  const auto fitness = [](const cga::Genome& genome) {
    float score = 0.0F;
    for (const auto& node : genome.nodes) score += static_cast<float>(node.inputs[0]);
    return score;
  };
  cga::EvolutionConfig config;
  config.population_size = 8;
  config.generations = 5;
  config.seed = 123;
  const auto first = cga::evolve(3, 10, 1, fitness, config);
  const auto second = cga::evolve(3, 10, 1, fitness, config);
  assert(first.best_fitness == second.best_fitness);
  assert(first.evaluations == second.evaluations);
  assert(first.best.outputs == second.best.outputs);
  assert(first.best.nodes.size() == second.best.nodes.size());
  for (std::size_t index = 0; index < first.best.nodes.size(); ++index) {
    assert(first.best.nodes[index].op == second.best.nodes[index].op);
    assert(first.best.nodes[index].inputs == second.best.nodes[index].inputs);
  }
  return 0;
}
