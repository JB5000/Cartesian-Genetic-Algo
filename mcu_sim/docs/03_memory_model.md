# Memory model

Each worker owns sixteen int32 registers, configurable persistent local RAM, a bytecode program, and one public output. The state snapshot is separate from private RAM and is read-only during execution of a round.

