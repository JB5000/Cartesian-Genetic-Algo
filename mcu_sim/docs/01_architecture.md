# Architecture

The MVP models independent workers that publish one int32 output per synchronous round. A controller boundary is represented by the simulator's publication step.

The implementation keeps worker programs, registers, local RAM, and public state separate. No private memory is implicitly visible to another worker.

