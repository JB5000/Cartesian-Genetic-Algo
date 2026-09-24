#include <cassert>

#include "cga/genome.hpp"
#include "cga/evaluate.hpp"

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
  return 0;
}
