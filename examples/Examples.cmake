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
)

if (MSVC)
    set(LIBRARIES "")
elseif(UNIX AND NOT APPLE)
    set(LIBRARIES ${CMAKE_DL_LIBS} m)
else ()
    set(LIBRARIES ${CMAKE_DL_LIBS})
endif()

# import_static_library
add_library(vanderpol_lib STATIC src/fmi3Functions.c src/model_common.c VanDerPol/model.c)
set_target_properties(vanderpol_lib PROPERTIES FOLDER examples)
target_include_directories(vanderpol_lib PRIVATE include VanDerPol)

add_executable(import_static_library examples/import_static_library.c)
set_target_properties (import_static_library PROPERTIES FOLDER examples)
target_include_directories(import_static_library PRIVATE include examples)
target_link_libraries(import_static_library vanderpol_lib)
set_target_properties(import_static_library PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# import_shared_library
add_executable(import_shared_library ${EXAMPLE_SOURCES} examples/import_shared_library.c)
set_target_properties (import_shared_library PROPERTIES FOLDER examples)
target_compile_definitions(import_shared_library PRIVATE DISABLE_PREFIX)
target_include_directories(import_shared_library PRIVATE include VanDerPol)
set_target_properties(import_shared_library PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)
target_link_libraries(import_shared_library ${LIBRARIES})

# cs_early_return
add_executable(cs_early_return
  ${EXAMPLE_SOURCES}
  BouncingBall/config.h
  examples/BouncingBall.c
  examples/cs_early_return.c
  examples/FMU.h
  examples/FMU.c
)
set_target_properties(cs_early_return PROPERTIES FOLDER examples)
target_compile_definitions(cs_early_return PRIVATE DISABLE_PREFIX)
target_include_directories(cs_early_return PRIVATE include BouncingBall)
target_link_libraries(cs_early_return ${LIBRARIES})
set_target_properties(cs_early_return PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# cs_event_mode
add_executable(cs_event_mode
  ${EXAMPLE_SOURCES}
  BouncingBall/config.h
  examples/BouncingBall.c
  examples/cs_event_mode.c
  examples/FMU.h
  examples/FMU.c
)
set_target_properties(cs_event_mode PROPERTIES FOLDER examples)
target_compile_definitions(cs_event_mode PRIVATE DISABLE_PREFIX)
target_include_directories(cs_event_mode PRIVATE include BouncingBall)
target_link_libraries(cs_event_mode ${LIBRARIES})
set_target_properties(cs_event_mode PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# cs_intermediate_update
add_executable(cs_intermediate_update
  ${EXAMPLE_SOURCES}
  BouncingBall/config.h
  examples/BouncingBall.c
  examples/cs_intermediate_update.c
  examples/FMU.h
  examples/FMU.c
)
set_target_properties(cs_intermediate_update PROPERTIES FOLDER examples)
target_compile_definitions(cs_intermediate_update PRIVATE DISABLE_PREFIX)
target_include_directories(cs_intermediate_update PRIVATE include BouncingBall)
target_link_libraries(cs_intermediate_update ${LIBRARIES})
set_target_properties(cs_intermediate_update PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# jacobian
add_executable(jacobian
  ${EXAMPLE_SOURCES}
  VanDerPol/config.h
  examples/jacobian.c
  examples/FMU.h
  examples/FMU.c
)
set_target_properties(jacobian PROPERTIES FOLDER examples)
target_compile_definitions(jacobian PRIVATE DISABLE_PREFIX)
target_include_directories(jacobian PRIVATE include VanDerPol)
target_link_libraries(jacobian ${LIBRARIES})
set_target_properties(jacobian PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# Examples
foreach (MODEL_NAME BouncingBall Stair)
    foreach (INTERFACE_TYPE me cs)
        set(TARGET_NAME ${MODEL_NAME}_${INTERFACE_TYPE})
        add_executable (${TARGET_NAME}
            ${EXAMPLE_SOURCES}
            ${MODEL_NAME}/config.h
            examples/FMU.h
            examples/FMU.c
            examples/simulate_${INTERFACE_TYPE}.c
            examples/${MODEL_NAME}.c
        )
        set_target_properties(${TARGET_NAME} PROPERTIES FOLDER examples)
        target_include_directories(${TARGET_NAME} PRIVATE include ${MODEL_NAME})
        target_compile_definitions(${TARGET_NAME} PRIVATE DISABLE_PREFIX)
        target_link_libraries(${TARGET_NAME} ${LIBRARIES})
        set_target_properties(${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY         temp
            RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
            RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
        )
    endforeach(INTERFACE_TYPE)
endforeach(MODEL_NAME)

# Connected CS
add_executable (connected_cs ${EXAMPLE_SOURCES} src/fmi3Functions.c src/model_common.c Feedthrough/model.c examples/FMU.h examples/FMU.c examples/connected_cs.c examples/Feedthrough.c)
set_target_properties(connected_cs PROPERTIES FOLDER examples)
target_include_directories(connected_cs PRIVATE include Feedthrough)
target_compile_definitions(connected_cs PRIVATE DISABLE_PREFIX)
target_link_libraries(connected_cs ${LIBRARIES})
set_target_properties(connected_cs PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# scs_synchronous
add_executable (scs_synchronous ${EXAMPLE_SOURCES} Clocks/config.h examples/FMU.h examples/FMU.c examples/scs_synchronous.c)
set_target_properties(scs_synchronous PROPERTIES FOLDER examples)
target_include_directories(scs_synchronous PRIVATE include Clocks)
target_link_libraries(scs_synchronous ${LIBRARIES})
set_target_properties(scs_synchronous PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

# Synchronous Supervisory Control Example
set (SUPERVISOR_FMUS Controller Plant Supervisor)

foreach (SUBMODEL ${SUPERVISOR_FMUS})
    add_library(Supervisory_${SUBMODEL} STATIC src/fmi3Functions.c src/model_common.c examples/SynchronousSupervisoryControl/submodels/${SUBMODEL}/model.c)
    set_target_properties(Supervisory_${SUBMODEL} PROPERTIES FOLDER examples)
    target_include_directories(Supervisory_${SUBMODEL} PRIVATE include examples/SynchronousSupervisoryControl/submodels/${SUBMODEL})
endforeach(SUBMODEL)

## ME Importer
add_executable(Supervisory_me examples/SynchronousSupervisoryControl/importers/me/synchronous_control_me.c)
set_target_properties(Supervisory_me PROPERTIES FOLDER examples)
target_include_directories(Supervisory_me PRIVATE include examples)
foreach (SUBMODEL ${SUPERVISOR_FMUS})
    target_link_libraries(Supervisory_me Supervisory_${SUBMODEL})
endforeach(SUBMODEL)
set_target_properties(Supervisory_me PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         temp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
)

if (WIN32)
    add_executable (scs_threaded ${EXAMPLE_SOURCES} Clocks/config.h examples/FMU.h examples/FMU.c examples/scs_threaded.c)
    set_target_properties(scs_threaded PROPERTIES FOLDER examples)
    target_include_directories(scs_threaded PRIVATE include Clocks)
    target_link_libraries(scs_threaded ${LIBRARIES})
    set_target_properties(scs_threaded PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY         temp
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
        RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
    )
endif ()
