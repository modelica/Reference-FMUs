set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(EXAMPLE_SOURCES
  include/fmi3Functions.h
  include/fmi3FunctionTypes.h
  include/fmi3PlatformTypes.h
  examples/util.h
)

set(MODEL_SOURCES
  VanDerPol/sources/fmi3Functions.c
  VanDerPol/sources/model.c
  VanDerPol/sources/slave.c
)

# cs_clocked
add_executable(cs_clocked
  ${EXAMPLE_SOURCES}
  Clocks/config.h
  Clocks/model.c
  src/fmi3Functions.c
  src/slave.c
  examples/cs_clocked.c
)
set_target_properties(cs_clocked PROPERTIES FOLDER examples)
target_include_directories(cs_clocked PRIVATE include Clocks)
if(UNIX AND NOT APPLE)
  target_link_libraries(cs_clocked m)
endif()

# cs_early_return
add_executable(cs_early_return
  ${EXAMPLE_SOURCES}
  BouncingBall/config.h
  BouncingBall/model.c
  src/fmi3Functions.c
  src/slave.c
  examples/cs_early_return.c
)
set_target_properties(cs_early_return PROPERTIES FOLDER examples)
target_include_directories(cs_early_return PRIVATE include BouncingBall)
if(UNIX AND NOT APPLE)
  target_link_libraries(cs_early_return m)
endif()

# co_simulation
add_library(slave1 STATIC ${EXAMPLE_SOURCES} src/fmi3Functions.c VanDerPol/model.c src/slave.c)
set_target_properties(slave1 PROPERTIES FOLDER examples)
target_compile_definitions(slave1 PRIVATE FMI3_FUNCTION_PREFIX=s1_)
target_include_directories(slave1 PRIVATE include VanDerPol)

add_library(slave2 STATIC ${EXAMPLE_SOURCES} src/fmi3Functions.c VanDerPol/model.c src/slave.c)
set_target_properties(slave2 PROPERTIES FOLDER examples)
target_compile_definitions(slave2 PRIVATE FMI3_FUNCTION_PREFIX=s2_)
target_include_directories(slave2 PRIVATE include VanDerPol)

add_executable(co_simulation ${EXAMPLE_SOURCES} examples/co_simulation.c)
set_target_properties(co_simulation PROPERTIES FOLDER examples)
target_include_directories(co_simulation PRIVATE include)
target_link_libraries(co_simulation slave1 slave2)

# jacobian
add_executable(jacobian ${EXAMPLE_SOURCES} src/fmi3Functions.c VanDerPol/model.c src/slave.c examples/jacobian.c)
set_target_properties (jacobian PROPERTIES FOLDER examples)
target_include_directories(jacobian PRIVATE include VanDerPol)
target_compile_definitions(jacobian PRIVATE DISABLE_PREFIX)

# model exchange
add_library(model STATIC ${EXAMPLE_SOURCES} src/fmi3Functions.c BouncingBall/model.c src/slave.c)
set_target_properties(model PROPERTIES FOLDER examples)
target_compile_definitions(model PRIVATE FMI3_FUNCTION_PREFIX=M_)
target_include_directories(model PRIVATE include BouncingBall)

add_executable (model_exchange ${EXAMPLE_SOURCES} src/fmi3Functions.c BouncingBall/model.c src/slave.c examples/model_exchange.c)
set_target_properties(model_exchange PROPERTIES FOLDER examples)
target_include_directories(model_exchange PRIVATE include BouncingBall)
target_link_libraries(model_exchange model)
target_compile_definitions(model_exchange PRIVATE DISABLE_PREFIX)
