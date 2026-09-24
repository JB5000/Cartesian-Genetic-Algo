# Design notes

## Genome layout

Inputs occupy values `[0, input_count)`. Node `i` occupies value
`input_count + i` and may reference only earlier values. This makes every
genome a directed acyclic graph and keeps execution a single forward pass.

## Active nodes

Only nodes reachable from an output are active. `Genome::active_nodes()` walks
backwards from the outputs and returns the active subgraph in execution order.
This makes inactive genes possible without adding runtime work.

## Evolution loop

The current baseline is a simple `(1 + λ)` strategy: evaluate a population,
keep the best genome, clone it and mutate the clones for the next generation.
The random generator is explicitly seeded so a run can be replayed.

## Fitness and checkpoints

Fitness is supplied as a callback, keeping the library independent of a
particular task. The included dataset helpers implement MSE, MAE and binary
accuracy. Checkpoints are plain text and contain the genome, fitness and
evaluation count, which makes them easy to inspect and archive.
