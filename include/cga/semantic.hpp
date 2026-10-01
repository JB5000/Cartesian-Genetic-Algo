#pragma once

#include <cmath>
#include <limits>
#include <vector>

#include "cga/dataset.hpp"

namespace cga {

// A semantic signature is the flattened output vector produced on a fixed
// dataset. Two genomes can have different graphs but identical signatures.
struct SemanticSignature {
  std::vector<float> values;
};

inline SemanticSignature semantic_signature(const Genome& genome,
                                            const Dataset& data) {
  SemanticSignature signature;
  for (const Sample& sample : data) {
    const Evaluation result = evaluate(genome, sample.inputs);
    signature.values.insert(signature.values.end(), result.outputs.begin(),
                            result.outputs.end());
  }
  return signature;
}

inline float semantic_distance(const SemanticSignature& left,
                               const SemanticSignature& right) {
  if (left.values.size() != right.values.size()) {
    return std::numeric_limits<float>::infinity();
  }
  if (left.values.empty()) return 0.0F;
  double total = 0.0;
  for (std::size_t index = 0; index < left.values.size(); ++index) {
    const double delta = static_cast<double>(left.values[index]) -
                         static_cast<double>(right.values[index]);
    total += delta * delta;
  }
  return static_cast<float>(total / static_cast<double>(left.values.size()));
}

inline float semantic_distance(const Genome& genome, const Dataset& data) {
  SemanticSignature target;
  for (const Sample& sample : data) {
    target.values.insert(target.values.end(), sample.targets.begin(),
                         sample.targets.end());
  }
  return semantic_distance(semantic_signature(genome, data), target);
}

}  // namespace cga
