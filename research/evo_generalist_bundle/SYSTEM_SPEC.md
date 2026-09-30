# System Specification

## 1. Core objective

Implement a domain-agnostic evolutionary program synthesizer / compact network learner.

Interface conceptually:

```c
train(problem, config) -> Genome
predict(genome, input[]) -> output[]
```

The same engine must support:
- supervised input→output approximation,
- classification,
- regression,
- boolean/circuit synthesis,
- control when fitness is supplied by an environment.

Do not encode task-specific solution hints into the ISA.

---

## 2. Worker architecture

Each worker owns one evolutionary trajectory.

Baseline search:

```text
parent
  |
mutate in-place
  |
evaluate
  |
accept -> commit
reject -> rollback delta
```

Use `(1+1)`, not an MCU-local population.

State per worker:
- current genome,
- current fitness,
- training champion,
- generalization champion,
- RNG state,
- stall/progress counters,
- delta log,
- active mask/cache where useful,
- per-run statistics.

No heap allocation in the hot loop.

---

## 3. Graph representation

Recommended starting point:
- 64 physical node slots,
- inactive slots allowed,
- 4 bytes per node.

```c
typedef struct {
    uint8_t op;
    uint8_t a;
    uint8_t b;
    uint8_t c;
} EvoNode;
```

`c` may encode:
- constant byte,
- module id,
- small parameter,
depending on `op`.

Keep source encoding feed-forward / acyclic.

Outputs are compact indices into node slots.

Use fixed arrays.

---

## 4. Generic ISA

Start with a small generic integer/fixed-point ISA.

Suggested set:
- PASS
- CONST
- ADD
- SUB
- MUL_FIXED
- AND
- OR
- XOR
- NOT
- ABS
- MIN
- MAX
- NEG
- SHL
- SHR
- EQ
- LT
- GT
- SELECT
- AVG
- CALL_MODULE

The exact first production ISA may be reduced if MCU timing/RAM requires it, but reductions must be benchmarked across multiple tasks.

Avoid task hints such as:
- carry,
- convolution,
- digit feature,
- game-specific rules.

---

## 5. Mutation

### 5.1 Location bias

Mutations that require a fitness evaluation should usually affect the phenotype.

Recommended initial bias:
- ~90% active graph,
- ~10% output/root mutations.

Inactive nodes are a neutral reservoir and may undergo free drift when they provably do not affect the output.

### 5.2 Mutation size

Keep small mutations dominant.

Use adaptive heavy-tail, for example conceptually:

```text
recent progress:
  90% k=1
   9% k=2
   1% k=4

moderate plateau:
  mostly 1/2
  occasional 4/8

deep plateau:
  keep many k=1 attempts
  but allow 4/8/16 structural jumps
```

Large-mutation scale may grow with active graph size, e.g. `sqrt(active_genes)`, but do not mutate a fixed high fraction of the genome on every candidate.

Fixed per-token probabilities of 1–4% were not a reliable default in experiments.

---

## 6. Delta/rollback

Never clone the whole genome per child on MCU.

Example:

```c
typedef struct {
    uint16_t index;
    uint8_t old_value;
} EvoDelta;
```

Mutation:
1. save old byte/value,
2. mutate parent in place,
3. evaluate,
4. accepted => forget delta,
5. rejected => restore in reverse order.

Host microbenchmark showed a modest speed improvement and definite RAM savings.

---

## 7. Fitness and compactness

Primary objective:
- task error / reward.

Secondary objectives:
- validation/generalization,
- fewer active nodes,
- fewer operations,
- smaller module footprint.

Do not let complexity dominate correctness. Use lexicographic or very small regularization.

---

## 8. Counterexample mining

Maintain:
- active train batch,
- challenge pool,
- validation subsets.

Periodically:
1. evaluate the current champion on challenge candidates,
2. identify hard failures,
3. replace easy active-train examples with hard examples,
4. continue evolution.

This mechanism is domain-agnostic.

For environment/control tasks, challenge cases can be initial conditions or scenarios where the controller performs poorly.

---

## 9. Five-validation generalization protection

Each run must maintain two distinct champions:

```text
BEST_TRAINING_CHAMPION
BEST_GENERALIZATION_CHAMPION
```

Use 5 validation subsets that are excluded from fitness training.

For classification:
- validation metric: accuracy or domain-appropriate loss.

For regression:
- orient metrics internally so higher score means better, e.g. `-MAE`.

Recommended selection order for the generalization champion:

1. higher `median(val1..val5)`,
2. higher `mean(val1..val5)`,
3. lower dispersion if desired,
4. fewer active nodes / lower inference cost.

The hidden test set is diagnostic only and must never be used by the controller or evolution.

A future production version should add a fresh confirmation gate before replacing the tile-global champion.

---

## 10. Modules

### 10.1 Representation

A module is a compact reusable function, initially 2 arguments.

A graph node may execute:

```text
CALL(module_id, source_a, source_b)
```

Modules themselves should be acyclic.

### 10.2 Auto-promotion

The engine may identify an active subgraph and abstract it into a module if:
- the subgraph has a small external interface,
- replacement by `CALL` is function-preserving on confirmation examples,
- the module fits fixed memory constraints.

### 10.3 Reuse

Default:
- auto-promotion enabled,
- explicit reuse rare,
- aggressive forcing disabled.

Future competitive-module score may include:
- number of active calls,
- number of distinct heads/users,
- generalization improvements,
- operations saved,
- lifetime,
- recent usefulness.

Unused modules may be recycled.

---

## 11. Multiple outputs

For difficult multi-output tasks, support:
- shared feature graph,
- output-specific heads,
- shared modules.

Do not make image-specific assumptions.

Conceptually:

```text
inputs
  |
shared graph
  |------ head 0 -> y0
  |------ head 1 -> y1
  ...
```

This improved the representational ceiling in Digits experiments.

---

## 12. 32-worker controller

A tile contains:
- 32 independent worker MCUs,
- 1 controller MCU.

The controller does not mutate individual genomes every generation.

It:
- assigns seeds and run budgets,
- receives worker champions/status,
- tracks 5-validation scores,
- preserves the tile-global generalization champion,
- allocates future worker runs across depth arms.

Recommended run-depth arms:
- 200k,
- 500k,
- 1M,
- 2M,
- optionally longer exploratory arms.

Do not use one universal fixed depth.

---

## 13. Adaptive scheduling

Reference conservative strategy:

1. pilot every depth arm,
2. record 5-validation mean/median of returned champions,
3. estimate arm quality conservatively,
4. favor reliable arms,
5. periodically force exploration of under-tested arms,
6. never use hidden test results.

The current reference implementation uses a lower-confidence estimate of the validation mean plus forced exploration.

Experiments indicate the controller can discover the useful ~500k region without being told it in advance.

---

## 14. Confirmation gate — next required feature

Before replacing the GLOBAL GENERALIZATION CHAMPION:

```text
worker candidate
  -> 5 validation subsets
  -> candidate appears better
  -> fresh confirmation subset
  -> accept or reject promotion
```

Purpose:
- reduce winner's curse / multiple-comparisons bias from many workers.

This is the next high-priority implementation item.

---

## 15. MCU constraints

Target:
- CH32V203C8T6-class MCU,
- roughly 20 KB RAM.

Guidelines:
- static arrays,
- no malloc,
- compact genome,
- fixed maximum modules,
- fixed maximum delta depth,
- cap CALL depth,
- keep datasets in flash/stream from controller where needed,
- use integer/fixed-point math,
- keep controller-worker protocol minimal.

Typical worker messages should be event-driven:
- heartbeat,
- run finished,
- new local generalization champion,
- error/status.

Do not synchronize all workers every generation.

---

## 16. Host/HPC version

The host implementation may add:
- OpenMP/process parallelism,
- SIMD,
- bit-packed truth tables,
- profiling,
- dataset loaders,
- exhaustive benchmark logging.

But host and MCU should share:
- genome encoding,
- opcode semantics,
- module semantics,
- deterministic RNG test vectors where possible.

---

## 17. Mandatory benchmark logging

For every run log:
- dataset/problem id,
- seed,
- run-depth arm,
- evaluation count,
- train fitness,
- val1..val5,
- val mean,
- val median,
- hidden-test metric (offline benchmark only),
- active nodes,
- active modules,
- active CALLs,
- ops/inference,
- runtime,
- genome bytes,
- controller arm decisions.

---

## 18. Experimental discipline

Every algorithm change must be tested with:
- equal compute,
- paired or independent predetermined seeds,
- same train/challenge/validation split,
- hidden test never used for selection,
- multiple task types.

Report at minimum:
- mean,
- median,
- best,
- lower quartile / failure rate,
- runtime,
- active graph size.

A feature that helps only one task should remain experimental until it generalizes.
