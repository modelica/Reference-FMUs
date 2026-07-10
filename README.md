# Reference FMUs

A set of hand-coded FMUs for development, testing, and debugging of the [Functional Mock-up Interface](https://fmi-standard.org/).

- [BouncingBall](BouncingBall) - a bouncing ball model with state events
- [Dahlquist](Dahlquist) - Dahlquist test equation
- [Feedthrough](Feedthrough) - all variable types
- [Resource](Resource) - load data from a file
- [Stair](Stair) - a counter with time events
- [StateSpace](StateSpace) - arrays and structural parameters
- [VanDerPol](VanDerPol) - Van der Pol test equation

:arrow_down: [Download the latest release.](https://github.com/modelica/Reference-FMUs/releases/latest/download/Reference-FMUs.zip)

## Repository structure

`<model>`
- `config.h` - model specific types and definitions
- `FMI{2|3}.xml` - model descriptions
- `model.c` - implementation of the model

`include`
- `fmi{2|3}Functions.h` - FMI header files
- `model.h` - generic model interface
- `cosimulation.h` - generic co-simulation interface

`src`
- `fmi{2|3}Functions.c` - FMI implementations
- `cosimulation.c` - generic co-simulation

`examples`
- `*.c` - various FMI 3.0 import examples
- `Examples.cmake` - CMake configuration for the example projects

## Build the FMUs

To build the FMUs you need [CMake](https://cmake.org/) and a supported [build tool](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html) e.g. Visual Studio, Xcode, or make:

```bash
cmake -B build .
cmake --build build
```

The FMUs will be created in `build/fmus/`.

## License

The code is released under the 2-Clause BSD license.
