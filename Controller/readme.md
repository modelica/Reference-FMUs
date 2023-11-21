# Controller

This FMU is part of a synchronous clock co-simulation example which consists of a [Controller](../Controller), a [Plant](../Plant), and a [Supervisor](../Supervisor) FMU.
It uses model exchange in FMI3.0.
The Controller fmu declares an input periodic clock,
and the supervisor has an output clock that triggers when a state event occurs.
The output clock of the supervisor is connected to another input clock of the controller.

More details and inputs/outputs of each FMU are in [synchronous_control_me.md](../examples/synchronous_control_me.md).
