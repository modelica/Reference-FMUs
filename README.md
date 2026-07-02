# Reference FMUs

A set of hand-coded FMUs for development, testing and debugging of the [Functional Mock-up Interface](https://fmi-standard.org/).

- [BouncingBall](BouncingBall) - a bouncing ball model with state events
- [Dahlquist](Dahlquist) - Dahlquist test equation
- [Feedthrough](Feedthrough) - all variable types
- [Resource](Resource) - load data from a file
- [Stair](Stair) - a counter with time events
- [StateSpace](StateSpace) - arrays and structural parameters
- [VanDerPol](VanDerPol) - Van der Pol test equation

You can download the pre-built Reference FMUs from [releases](https://github.com/modelica/Reference-FMUs/releases).

## Repository structure

`<model>`
- `config.h` - model specific types and definitions
- `FMI{1CS|1ME|2|3}.xml` - model descriptions
- `model.c` - implementation of the model

`include`
- `fmi{|2|3}Functions.h` - FMI header files
- `model.h` - generic model interface
- `cosimulation.h` - generic co-simulation interface

`src`
- `fmi{1|2|3}Functions.c` - FMI implementations
- `cosimulation.c` - generic co-simulation

`examples`
- `*.c` - various FMI 3.0 import examples
- `Examples.cmake` - CMake configuration for the example projects

## Build the FMUs

To build the FMUs you need [CMake](https://cmake.org/) and a supported [build tool](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html) e.g. Visual Studio &GreaterEqual; 2013 , Xcode or make:

* download or clone the repository
* open the [CMakeGUI](https://cmake.org/runningcmake/)
* click `Browse Source...` and select the cloned or downloaded and extracted repository (that contains `CMakeLists.txt`)
* click `Browse Build...` and select the folder where you want build the FMUs
* click `Configure` and select the generator for your IDE / build tool
* select the `FMI_VERSION` you want to build
* click `Generate` to generate the project files
* click `Open Project` or open the project in your build tool
* build the project

The FMUs will be in the `dist` folder inside the selected build folder.

## License

Copyright &copy; 2026, Modelica Association Project "FMI".
All rights reserved.
The code is released under the [2-Clause BSD License](LICENSE.txt).
