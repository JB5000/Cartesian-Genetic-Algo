#include <cassert>

#include "cga/genome.hpp"
#include "cga/evaluate.hpp"
#include "cga/mutation.hpp"
#include "cga/dataset.hpp"
#include "cga/evolution.hpp"
#include "cga/checkpoint.hpp"
#include "cga/metrics.hpp"

int main() {
  cga::Genome genome;
  genome.input_count = 2;
  genome.nodes = {{cga::OpCode::Add, {0, 1}},
                  {cga::OpCode::Multiply, {2, 1}}};
  genome.outputs = {3};
  assert(genome.valid());
  const auto active = genome.active_nodes();
  assert(active.size() == 2);
  assert(active[0] == 0);
  assert(active[1] == 1);
  const auto result = cga::evaluate(genome, {3.0F, 2.0F});
  assert(result.outputs.size() == 1);
  assert(result.outputs[0] == 10.0F);
  assert(result.active_nodes == 2);
  std::mt19937 rng(42);
  auto random = cga::random_genome(2, 8, 1, rng);
  assert(random.valid());
  const auto changed = cga::mutate(random, rng);
  assert(random.valid());
  assert(changed > 0);
  cga::Dataset data{{{0.0F, 0.0F}, {0.0F}},
                    {{1.0F, 0.0F}, {1.0F}},
                    {{0.0F, 1.0F}, {1.0F}},
                    {{1.0F, 1.0F}, {2.0F}}};
  assert(cga::mean_squared_error(genome, data) >= 0.0F);
  assert(cga::mean_absolute_error(genome, data) >= 0.0F);
  assert(cga::active_node_ratio(genome) > 0.0F);
  const auto signature = cga::semantic_signature(genome, data);
  assert(signature.values.size() == data.size());
  assert(cga::semantic_distance(genome, data) >= 0.0F);
  std::mt19937 semantic_rng(17);
  cga::SemanticMutationConfig semantic_config;
  semantic_config.trials = 3;
  semantic_config.mutation.node_rate = 0.25;
  semantic_config.mutation.output_rate = 0.25;
  const auto semantic_result = cga::semantic_mutate(
      genome, data, semantic_rng, semantic_config);
  assert(genome.valid());
  assert(semantic_result.after <= semantic_result.before);
  cga::EvolutionConfig config;
  config.population_size = 6;
  config.generations = 3;
  config.seed = 7;
  const auto evolved = cga::evolve(2, 5, 1,
      [&](const cga::Genome& candidate) {
        return cga::mean_squared_error(candidate, data);
      }, config);
  assert(evolved.evaluations == 18);
  assert(evolved.best.valid());
  assert(evolved.history.size() == 3);
  const std::string checkpoint_path = "/tmp/cga_smoke_checkpoint.ckpt";
  cga::save_checkpoint(checkpoint_path, {evolved.best, evolved.best_fitness,
                                         evolved.evaluations});
  const auto restored = cga::load_checkpoint(checkpoint_path);
  assert(restored.genome.valid());
  assert(restored.genome.nodes.size() == evolved.best.nodes.size());
  assert(restored.evaluations == evolved.evaluations);
  return 0;
}
