# Implementation Plan

## Phase 0 — Clean repository

Create:

```text
include/
  evo_types.h
  evo_ops.h
  evo_genome.h
  evo_mutation.h
  evo_fitness.h
  evo_validation.h
  evo_modules.h
  evo_controller.h
  evo_protocol.h

src/
  evo_ops.c
  evo_genome.c
  evo_mutation.c
  evo_validation.c
  evo_modules.c
  evo_controller.c

host/
  runner.cpp
  benchmark.cpp
  dataset_adapter.cpp

mcu/
  worker_main.c
  controller_main.c

tests/
  test_ops.cpp
  test_delta.cpp
  test_genome_roundtrip.cpp
  test_module_call.cpp
  test_validation.cpp
  test_controller.cpp

benchmarks/
  logic/
  tabular/
  distillation/
```

Do not copy the experimental sources into production wholesale. Extract behavior with tests.

## Phase 1 — Core deterministic runtime

Implement:
- 4-byte node,
- fixed genome,
- scalar evaluator,
- exact opcode semantics,
- active graph traversal,
- output decoding,
- deterministic RNG.

Add unit tests with fixed known genomes.

## Phase 2 — `(1+1)` worker

Implement:
- in-place mutation,
- delta log,
- rollback,
- current fitness,
- training champion,
- fixed evaluation budget.

Verify no heap allocation.

## Phase 3 — Mutation policies

Implement:
- active bias,
- output/root mutation,
- neutral inactive drift,
- adaptive heavy-tail mutation count.

Expose policy counters for benchmarks.

## Phase 4 — Generalization

Implement:
- challenge pool,
- counterexample mining,
- 5 validation subsets,
- val mean + val median,
- separate generalization champion.

Ensure hidden test is inaccessible from evolution code.

## Phase 5 — Modules

Implement:
- fixed module table,
- CALL opcode,
- max call depth,
- auto-promotion,
- function-preserving confirmation,
- rare explicit reuse behind an experimental flag.

## Phase 6 — Controller

Implement worker result packet:
- seed,
- arm/depth,
- eval count,
- val1..val5,
- mean/median,
- compactness,
- genome/module hash,
- optional genome payload only for promising candidates.

Implement arms:
- 200k,
- 500k,
- 1M,
- 2M.

Implement:
- pilot phase,
- conservative lower-confidence arm score,
- forced exploration,
- tile-global champion.

## Phase 7 — Confirmation gate

When a candidate beats the tile global champion:
- evaluate on fresh confirmation examples,
- only then promote globally.

Track rejection rate.

## Phase 8 — Host benchmark suite

Required benchmarks:
- boolean multiplier / logic synthesis,
- hidden symbolic function,
- Breast Cancer,
- Wine,
- Diabetes,
- Digits,
- one control/environment task.

Run equal-compute comparisons:
- fixed arm,
- adaptive controller,
- modules OFF/ON,
- confirmation OFF/ON.

## Phase 9 — MCU port

Port only after host tests pass.

Measure:
- RAM static usage,
- stack high-water mark,
- evaluations/s,
- joules/evaluation if possible,
- controller bus traffic,
- worst-case CALL stack depth.

## Phase 10 — Freeze firmware ABI

Freeze:
- node format,
- genome serialization,
- module serialization,
- worker/controller packet structs,
- opcode IDs.

Version all serialized objects.
