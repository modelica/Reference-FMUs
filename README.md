<p align="center">
  <img src="logo.svg" alt="Test FMUs logo" width="300" height="115">
</p>

# Test FMUs

A set of test models for development, testing and debugging of the [Functional Mock-up Interface](https://fmi-standard.org/).

- [BouncingBall](BouncingBall) - a bouncing ball model with state events
- [Dahlquist](Dahlquist) - Dahlquist test equation
- [Feedthrough](Feedthrough) - all variable types
- [Resource](Resource) - load data from a file
- [Stair](Stair) - a counter with time events
- [VanDerPol](VanDerPol) - Van der Pol test equation

## Repository structure

`<model>`
- `config.h` - model specific types and definitions
- `FMI*.xml` - model descriptions
- `model.c` - implementation of the model

`include`
- `fmi*.h` - FMI header files
- `model.h` - generic model interface
- `slave.h` - generic co-simulation interface
- `solver.h` - solver interface

`src`
- `euler.c` - forward Euler solver
- `fmi[1,2,3].c` - FMI implementation
- `slave.c` - generic co-simulation

## Build the FMUs

To build the FMUs you need [CMake](https://cmake.org/) and a supported [build tool](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html) e.g. Visual Studio, Xcode or make:

- download or clone the repository

- open the [CMakeGUI](https://cmake.org/runningcmake/)

- click `Browse Source...` and select the cloned or downloaded and extracted repository (that contains `CMakeLists.txt`)

- click `Browse Build...` and select the folder where you want build the FMUs

- click `Configure` and select the generator for your IDE / build tool

- select the `FMI_VERSION` you want to build and optionally the `FMI_TYPE` (only for FMI 1.0)

- click `Generate` to generate the project files

- click `Open Project` or open the project in your build tool

- build the project

The FMUs will be in the `dist` folder inside the selected build folder.

## License and Attribution

Copyright &copy; 2019 Dassault Systemes.
All rights reserved.
The code is released under the [2-Clause BSD License](LICENSE.txt).
The Test FMUs are forked from the [FMU SDK](https://github.com/qtronic/fmusdk) by QTronic.
The stethoscope icon in the logo by [srip](https://www.flaticon.com/authors/srip) is licensed [CC-BY 3.0](http://creativecommons.org/licenses/by/3.0/).
