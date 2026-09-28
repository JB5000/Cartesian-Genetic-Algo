# CGP MCU simulator MVP

This C++20 project validates a synchronous, double-buffered network of independent bytecode nodes. It deliberately separates measured CPU results from an analytical prediction for the future MCU PCB.

## Build and run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/cgp_mcu_demo
./build/cgp_mcu_benchmark results/sweep.csv
```

`cgp_mcu_benchmark` measures a sequential CPU interpreter and persistent pools of 2/4/8/16 threads with identical programs. It also produces an MCU prediction sweep for 4–256 nodes, global-bus and clustered-16 topology, bus 2/5/10/20 MHz, clock 48/72/96/144 MHz, and 16/32-bit public state. CPU rows are measurements on the host; `mcu_*` rows are predictions only.

## Evolution simulation for the proposed PCB

`cgp_mcu_evolution` models one evolutionary cycle for the intended architecture:
the Raspberry Pi selects a parent, the Controller sends one mutation delta to
each of 14 Workers, the Workers evaluate their candidates in parallel, and
Fitness selects the best result. The included target is `x*x+3*x+1`, so the
search has a known answer. It records the fitness curve and distinguishes host
runtime from the estimated PCB timing.

```bash
./build/cgp_mcu_evolution --generations 20000 --seed 1 --csv results/evolution.csv
```

The default PCB estimate uses a 144 MHz worker, 8-bit 1 MHz state bus, and 8
cycles per bytecode instruction. These are explicit assumptions, not measured
CH32V203 timings. Re-run with measured values using `--worker-mhz`,
`--bus-mhz`, and `--cycles-per-instruction`.

`cgp_mcu_physical_twin` is the event-level protocol model for the proposed
PCB. It has 14 Worker MCUs, one Controller, one Fitness MCU and a Raspberry Pi
host. Each virtual generation includes Pi framing, mutation and INPUT
broadcast, parallel Worker execution, BUSY/DONE barrier, 14 fixed DATA8 slots
with 32-bit state and CRC16, READY, COMMIT/ACK and local Fitness. It starts at
the 100 kHz DATA8 bring-up rate specified in the hardware documents and emits
one timing row per generation.

```bash
./build/cgp_mcu_physical_twin --generations 1000 --csv results/physical_twin.csv
```

It can inject a bit error to model CRC/retry behaviour and accepts measured
values for clock, bus rate, instruction cost, series resistance and load:

```bash
./build/cgp_mcu_physical_twin --bus-khz 100 --worker-mhz 144 \
  --cycles-per-instruction 8 --series-ohm 470 --load-pf 150 \
  --bit-error-probability 0 --csv results/physical_twin.csv
```

The default physical-twin profile is the compact Rev A protocol: one CRC16
over the complete 14-slot snapshot and CRC-protected READY/COMMIT bitmasks.
For the slower diagnostic profile, use `--crc-per-slot 1 --individual-acks 1`.

With the documented 470 ohm series resistor and 150 pF load estimate, the
10–90% RC rise time is about 155 ns. A 30% unit-interval margin gives an
analytical ceiling near 1.9 MHz, so the model caps the recommended bring-up
rate at **1 MHz**. This is a starting frequency, not an electrical guarantee;
the assembled board must be measured before increasing it.

The 20 KiB accounting model includes a 20% reserve, 5 KiB runtime reserve,
double state, 24-gene program, registers, input/target staging, protocol
buffers and a 2 KiB stack margin. It leaves 8,432 B in this model. The actual
CH32V203 firmware still needs a linker map and stack high-water measurement.

## Genetic challenge suite

`cgp_mcu_challenges` runs the same 14-worker evolutionary loop over known
targets: polynomial and absolute-value regression, two-input XOR, three-input
parity, a 2-to-1 multiplexer and one harder four-input nonlinear target. It
reports success rate and median generations and stores every seed in a CSV.

```bash
./build/cgp_mcu_challenges --seeds 32 --generations 10000 \
  --csv results/challenges.csv
```

To run only the single difficult challenge:

```bash
./build/cgp_mcu_challenges --challenge hard --seeds 32 \
  --generations 10000 --csv results/hard_challenge.csv
```

For the extreme stress test with six inputs and a deeply nested nonlinear
target, use `--challenge extreme`. It is expected to expose search stagnation
with the current local-mutation policy; the report records that result.

These challenges test the search algorithm and the worker/fitness loop. Their
wall time is estimated separately by `cgp_mcu_physical_twin`.

## Neural-network style challenge

`cgp_mcu_nn_challenge` evolves one small network for a nonlinear next-token
task. It receives four context values `x0..x3` and predicts a binary `y`; all
14 workers mutate the same network and Fitness selects on training accuracy.
The CSV records train accuracy, held-out test accuracy and the best test value
over time.

```bash
./build/cgp_mcu_nn_challenge --seconds 300 --seed 7 \
  --csv results/nn_challenge_5min.csv
```

For parallel independent populations on Slurm:

```bash
mkdir -p logs
sbatch scripts/slurm_nn_array.sh
```

## Model and interpretation

The default realistic model uses a 16-bit bus, 10 MHz bus clock, 144 MHz worker clock and configurable per-instruction cost (default 2 cycles). A round is `max(worker compute) + serialized publication + sync`. At 16 nodes and 32-bit state, publication needs 32 bus transfers before overhead. The default timing scenarios add GPIO, turnaround, barrier and controller costs; edit `timing()` or `HardwareModel` to match measurements from a prototype.

The key crossover is the instruction count at which `communication_percentage` falls below 50% in the CSV. Under the default realistic model it first occurs at **1024 bytecode instructions/node** in the prescribed sweep (the continuous estimate is about 667) at 144 MHz and 10 MHz bus. Below this, improving/bypassing the bus and synchronization dominates; above it, worker compute dominates and adding useful work between publications can justify the architecture. This does not establish an advantage over a modern CPU: the measured CPU rows are the proper comparison and normally remain much faster per watt-unknown host core. The PCB is most plausible when deterministic distributed local state, I/O locality, and massive independent evaluation matter more than raw interpreter throughput.

The highest-impact hardware improvements are a wider/faster broadcast fabric, DMA-assisted state publication, lower barrier/controller latency, and cluster-local buses once global-bus serialization becomes dominant. The included global model scales directly with node count; `HardwareModel::clustered` offers a first-order clustered estimate, not a routing implementation.

## Coverage

Tests cover snapshot atomicity/order independence, a valid two-node recurrence, local-memory isolation, functional-vs-parallel bit identity, and the expected compute/communication trend. Evolutionary CGP is intentionally deferred until the requested architectural and timing baseline is validated.
