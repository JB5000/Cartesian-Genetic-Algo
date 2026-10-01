# Semantic GP host operator

The semantic layer evaluates a genome on a fixed set of input/output examples
and flattens all produced outputs into a semantic signature. The graph
topology is not part of the signature: two different graphs with the same
outputs are semantically equivalent on that dataset.

`cga::semantic_mutate` samples several mutated candidates and commits only the
candidate with the smallest mean squared semantic distance. This is a simple
host-side hill-climbing operator for experiments; it does not replace the
MCU `(1+1)` loop or its delta/rollback implementation.

Build and run the deterministic XOR smoke benchmark with:

```bash
cmake -S . -B build -DCGA_BUILD_BENCHMARKS=ON
cmake --build build --target cga_semantic_xor
./build/cga_semantic_xor
```

The benchmark reports initial and final semantic MSE and the number of active
nodes. It is deliberately small so that mutation policies can be compared
before moving them into the bounded MCU runtime.
