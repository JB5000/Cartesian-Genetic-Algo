# Codex Implementation Prompt

You are implementing a production-quality version of the experimental evolutionary system contained in this repository.

## Read first

1. `SYSTEM_SPEC.md`
2. `IMPLEMENTATION_PLAN.md`
3. `docs/BENCHMARK_SUMMARY.md`
4. the code in `src/reference/`
5. `src/mcu/evo_cgp_mcu_runtime.h`
6. `tools/adaptive_controller_reference.py`

## Primary target

Create a clean C/C++ implementation of a general evolutionary input→output system suitable for eventual deployment on:
- CH32V203C8T6-class worker MCUs,
- 32 workers + 1 controller per tile,
- roughly 20 KB RAM per worker MCU.

Do NOT simply concatenate the experimental source files.

The experimental files contain useful mechanisms but were written for isolated benchmarks. Build a coherent implementation with tests and clear interfaces.

## Required architecture

Implement:

### Worker
- fixed compact CGP-like graph,
- 4-byte nodes,
- 64 node baseline,
- `(1+1)` evolution,
- in-place mutation,
- delta/rollback,
- active-node bias,
- adaptive heavy-tail mutation sizes,
- neutral drift for inactive nodes,
- counterexample mining,
- training champion,
- five-validation generalization champion,
- optional module table and CALL opcode.

### Generalization
Maintain 5 validation subsets.

Generalization champion ordering:
1. higher median across the five validation metrics,
2. higher mean,
3. lower validation dispersion if useful,
4. smaller active graph / lower inference cost.

The hidden test set MUST NOT be available to the evolutionary search or controller.

### Modules
- fixed-size module library,
- 2-argument CALL initially,
- automatic function-preserving subgraph promotion,
- rare explicit reuse behind a feature flag,
- aggressive module reuse disabled.

### Controller
Support run-depth arms:
- 200k
- 500k
- 1M
- 2M

Implement a conservative adaptive scheduler:
- pilot every arm,
- collect five-validation champion scores,
- estimate arm quality conservatively,
- prefer reliable arms,
- force periodic exploration,
- keep a tile-global generalization champion.

### Confirmation gate
This is the main new feature to implement beyond the current references.

When a worker candidate would replace the global generalization champion:
1. evaluate it on a fresh confirmation subset not used in training/challenge/VAL1..VAL5,
2. compare it against the current global champion on the same confirmation data,
3. promote only if the result is confirmed,
4. log acceptance/rejection.

The confirmation examples must not leak into future training unless explicitly moved there by a separately defined policy.

## Hard constraints

- no heap allocation in MCU hot loop,
- no whole-genome child copy,
- deterministic reproducible seeds,
- bounded call depth,
- fixed-size arrays,
- explicit serialization format,
- versioned genome/module ABI,
- tests for rollback correctness,
- tests for module-call equivalence,
- tests proving hidden-test isolation.

## Benchmark requirements

Create a host benchmark runner that can run:
- logic/circuit synthesis,
- symbolic hidden function,
- Breast Cancer,
- Wine,
- Diabetes,
- Digits,
- at least one control environment adapter.

For every algorithm change, compare with equal compute.

Minimum logged fields:
- seed,
- run-depth arm,
- evaluations,
- training fitness,
- val1..val5,
- validation median,
- validation mean,
- confirmation score,
- hidden-test score (benchmark harness only),
- active nodes,
- active modules,
- active CALLs,
- ops/inference,
- runtime.

## Acceptance tests for this implementation

1. Core evaluator matches fixed reference genomes.
2. Delta rollback exactly restores genomes.
3. Inactive neutral mutations do not alter predictions.
4. Module promotion is function-preserving on its confirmation set.
5. Generalization champion never reads hidden-test data.
6. Adaptive controller can allocate runs across all four arms.
7. Controller always retains some exploration.
8. Confirmation gate blocks at least some false promotions in a synthetic noisy-validation test.
9. Host build runs all benchmark adapters.
10. MCU build compiles without heap usage.

## Do not optimize prematurely

First reproduce correctness and benchmark behavior.

Only then profile:
- active-mask cost,
- evaluator,
- mutation,
- validation,
- module calls,
- controller protocol.

Maintain a CHANGELOG of every behavioral change and its benchmark result.
