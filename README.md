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

## License

MIT.
