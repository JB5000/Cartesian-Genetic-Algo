#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <random>
#include <utility>
#include <vector>

#include "cga/mutation.hpp"

namespace cga {

struct EvolutionConfig {
  std::size_t population_size = 32;
  std::size_t generations = 100;
  MutationConfig mutation{};
  std::uint32_t seed = 1;
};

struct EvolutionResult {
  Genome best;
  float best_fitness = std::numeric_limits<float>::infinity();
  std::size_t evaluations = 0;
  std::vector<float> history;
};

using Fitness = std::function<float(const Genome&)>;

inline EvolutionResult evolve(std::size_t inputs, std::size_t nodes,
                              std::size_t outputs, const Fitness& fitness,
                              EvolutionConfig config = {}) {
  if (config.population_size == 0) throw std::invalid_argument("population cannot be empty");
  std::mt19937 rng(config.seed);
  EvolutionResult result;
  result.best = random_genome(inputs, nodes, outputs, rng);

  std::vector<Genome> population;
  population.reserve(config.population_size);
  for (std::size_t index = 0; index < config.population_size; ++index) {
    population.push_back(random_genome(inputs, nodes, outputs, rng));
  }

  for (std::size_t generation = 0; generation < config.generations; ++generation) {
    auto best_it = population.begin();
    float best_fitness = fitness(*best_it);
    ++result.evaluations;
    for (auto it = population.begin() + 1; it != population.end(); ++it) {
      const float candidate_fitness = fitness(*it);
      ++result.evaluations;
      if (candidate_fitness < best_fitness) {
        best_fitness = candidate_fitness;
        best_it = it;
      }
    }
    if (best_fitness < result.best_fitness) {
      result.best_fitness = best_fitness;
      result.best = *best_it;
    }
    result.history.push_back(result.best_fitness);

    std::vector<Genome> next;
    next.reserve(config.population_size);
    next.push_back(*best_it);
    while (next.size() < config.population_size) {
      Genome child = *best_it;
      mutate(child, rng, config.mutation);
      next.push_back(std::move(child));
    }
    population = std::move(next);
  }
  return result;
}

}  // namespace cga
