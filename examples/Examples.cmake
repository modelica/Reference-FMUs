set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(EXAMPLE_SOURCES
  include/fmi3Functions.h
  include/fmi3FunctionTypes.h
  include/fmi3PlatformTypes.h
  include/model.h
  examples/util.h
)

set(MODEL_SOURCES
  VanDerPol/sources/fmi3Functions.c
  VanDerPol/sources/model.c
  VanDerPol/sources/cosimulation.c
)

# import_static_library
add_executable(import_static_library ${EXAMPLE_SOURCES} src/fmi3Functions.c VanDerPol/model.c src/cosimulation.c examples/import_static_library.c)
set_target_properties (import_static_library PROPERTIES FOLDER examples)
target_include_directories(import_static_library PRIVATE include VanDerPol)
set_target_properties(import_static_library PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# import_shared_library
add_executable(import_shared_library ${EXAMPLE_SOURCES} src/fmi3Functions.c VanDerPol/model.c src/cosimulation.c examples/import_shared_library.c)
set_target_properties (import_shared_library PROPERTIES FOLDER examples)
target_include_directories(import_shared_library PRIVATE include VanDerPol)
set_target_properties(import_shared_library PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)
target_link_libraries(import_shared_library ${CMAKE_DL_LIBS})

# bcs_early_return
add_executable(bcs_early_return
  ${EXAMPLE_SOURCES}
  BouncingBall/config.h
  BouncingBall/model.c
  src/fmi3Functions.c
  src/cosimulation.c
  examples/bcs_early_return.c
)
set_target_properties(bcs_early_return PROPERTIES FOLDER examples)
target_compile_definitions(bcs_early_return PRIVATE DISABLE_PREFIX)
target_include_directories(bcs_early_return PRIVATE include BouncingBall)
if(UNIX AND NOT APPLE)
  target_link_libraries(bcs_early_return m)
endif()
set_target_properties(bcs_early_return PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# bcs_intermediate_update
add_executable(bcs_intermediate_update
  ${EXAMPLE_SOURCES}
  BouncingBall/config.h
  BouncingBall/model.c
  src/fmi3Functions.c
  src/cosimulation.c
  examples/bcs_intermediate_update.c
)
set_target_properties(bcs_intermediate_update PROPERTIES FOLDER examples)
target_compile_definitions(bcs_intermediate_update PRIVATE DISABLE_PREFIX)
target_include_directories(bcs_intermediate_update PRIVATE include BouncingBall)
if(UNIX AND NOT APPLE)
  target_link_libraries(bcs_intermediate_update m)
endif()
set_target_properties(bcs_intermediate_update PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# hcs_early_return
add_executable(hcs_early_return
  ${EXAMPLE_SOURCES}
  BouncingBall/config.h
  BouncingBall/model.c
  src/fmi3Functions.c
  src/cosimulation.c
  examples/hcs_early_return.c
)
set_target_properties(hcs_early_return PROPERTIES FOLDER examples)
target_compile_definitions(hcs_early_return PRIVATE DISABLE_PREFIX)
target_include_directories(hcs_early_return PRIVATE include BouncingBall)
if(UNIX AND NOT APPLE)
  target_link_libraries(hcs_early_return m)
endif()
set_target_properties(hcs_early_return PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# co_simulation
add_library(model1 STATIC ${EXAMPLE_SOURCES} src/fmi3Functions.c VanDerPol/model.c src/cosimulation.c)
set_target_properties(model1 PROPERTIES FOLDER examples)
target_compile_definitions(model1 PRIVATE FMI3_FUNCTION_PREFIX=s1_)
target_include_directories(model1 PRIVATE include VanDerPol)

add_library(model2 STATIC ${EXAMPLE_SOURCES} src/fmi3Functions.c VanDerPol/model.c src/cosimulation.c)
set_target_properties(model2 PROPERTIES FOLDER examples)
target_compile_definitions(model2 PRIVATE FMI3_FUNCTION_PREFIX=s2_)
target_include_directories(model2 PRIVATE include VanDerPol)

add_executable(co_simulation ${EXAMPLE_SOURCES} examples/co_simulation.c)
set_target_properties(co_simulation PROPERTIES FOLDER examples)
target_include_directories(co_simulation PRIVATE include)
target_link_libraries(co_simulation model1 model2)
set_target_properties(co_simulation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# jacobian
add_executable(jacobian ${EXAMPLE_SOURCES} src/fmi3Functions.c VanDerPol/model.c src/cosimulation.c examples/jacobian.c)
set_target_properties (jacobian PROPERTIES FOLDER examples)
target_include_directories(jacobian PRIVATE include VanDerPol)
target_compile_definitions(jacobian PRIVATE DISABLE_PREFIX)
set_target_properties(jacobian PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# model exchange
add_library(BouncingBall_static STATIC ${EXAMPLE_SOURCES} src/fmi3Functions.c BouncingBall/model.c src/cosimulation.c)
set_target_properties(BouncingBall_static PROPERTIES FOLDER examples)
target_compile_definitions(BouncingBall_static PRIVATE FMI3_FUNCTION_PREFIX=M_)
target_include_directories(BouncingBall_static PRIVATE include BouncingBall)

add_executable (BouncingBall_me ${EXAMPLE_SOURCES} src/fmi3Functions.c BouncingBall/model.c src/cosimulation.c examples/model_exchange.c)
set_target_properties(BouncingBall_me PROPERTIES FOLDER examples)
target_include_directories(BouncingBall_me PRIVATE include BouncingBall)
target_link_libraries(BouncingBall_me BouncingBall_static)
target_compile_definitions(BouncingBall_me PRIVATE DISABLE_PREFIX)
set_target_properties(BouncingBall_me PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# Scheduled Co-Simulation
add_executable(scs_synchronous ${EXAMPLE_SOURCES} src/fmi3Functions.c Clocks/model.c src/cosimulation.c examples/scs_synchronous.c Clocks/FMI3.xml Clocks/config.h)
set_target_properties(scs_synchronous PROPERTIES FOLDER examples)
target_include_directories(scs_synchronous PRIVATE include Clocks)
target_compile_definitions(scs_synchronous PRIVATE DISABLE_PREFIX)
set_target_properties(scs_synchronous PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

if (WIN32)
  add_executable(scs_threaded ${EXAMPLE_SOURCES} src/fmi3Functions.c Clocks/model.c src/cosimulation.c examples/scs_threaded.c Clocks/FMI3.xml Clocks/config.h)
  set_target_properties(scs_threaded PROPERTIES FOLDER examples)
  target_compile_definitions(scs_threaded PRIVATE DISABLE_PREFIX)
  target_include_directories(scs_threaded PRIVATE include Clocks)
  target_compile_definitions(scs_threaded PRIVATE DISABLE_PREFIX)
  set_target_properties(scs_threaded PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY         temp
      RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
      RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
  )
endif ()
