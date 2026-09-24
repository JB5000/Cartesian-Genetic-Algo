# Cartesian Genetic Algo

A small, dependency-free C++17 implementation of Cartesian Genetic Programming (CGP).

The project evolves compact directed acyclic graphs made of primitive functions. It is intended for experiments where topology, active-node count, mutation behaviour and deterministic replay matter.

## Status

The current milestone includes graph execution, mutation, deterministic evolution, a mean-squared-error dataset evaluator and an XOR example.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/cga_xor
```

To build the optional throughput benchmark:

```bash
cmake -S . -B build -DCGA_BUILD_BENCHMARKS=ON
cmake --build build
./build/cga_throughput
```

The public API is header-only under `include/cga/`; applications can include
everything with `#include <cga/cga.hpp>`. See
[`docs/design.md`](docs/design.md) for the genome layout and evolution model.

## License

MIT.
