#pragma once

#include <cstddef>
#include <limits>
#include <random>
#include <utility>

#include "cga/mutation.hpp"
#include "cga/semantic.hpp"

namespace cga {

struct SemanticMutationConfig {
  std::size_t trials = 4;
  MutationConfig mutation{};
};

struct SemanticMutationResult {
  float before = std::numeric_limits<float>::infinity();
  float after = std::numeric_limits<float>::infinity();
  std::size_t changed_genes = 0;

  bool improved() const noexcept { return after < before; }
};

// Host-side semantic hill-climbing operator. It samples several mutated
// genotypes and commits only the candidate with the best full output vector.
// The MCU (1+1) worker can use the same acceptance rule with a delta log.
inline SemanticMutationResult semantic_mutate(
    Genome& genome, const Dataset& data, std::mt19937& rng,
    SemanticMutationConfig config = {}) {
  genome.require_valid();
  SemanticMutationResult result;
  result.before = semantic_distance(genome, data);
  Genome best = genome;
  std::size_t best_changes = 0;
  float best_distance = result.before;

  for (std::size_t trial = 0; trial < config.trials; ++trial) {
    Genome candidate = genome;
    const std::size_t changed = mutate(candidate, rng, config.mutation);
    const float distance = semantic_distance(candidate, data);
    if (distance < best_distance) {
      best_distance = distance;
      best = std::move(candidate);
      best_changes = changed;
    }
  }

  if (best_distance < result.before) {
    genome = std::move(best);
    result.changed_genes = best_changes;
    result.after = best_distance;
  } else {
    result.after = result.before;
  }
  return result;
}

}  // namespace cga
