set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(EXAMPLE_SOURCES
    include/FMI.h
    include/FMI${FMI_VERSION}.h
    include/model.h
    examples/util.h
    src/FMI.c
    src/FMI${FMI_VERSION}.c
)

if (MSVC)
    set(LIBRARIES "")
elseif(UNIX AND NOT APPLE)
    set(LIBRARIES ${CMAKE_DL_LIBS} m)
else ()
    set(LIBRARIES ${CMAKE_DL_LIBS})
endif()

if (${FMI_VERSION} EQUAL 3)

    # import_static_library
    add_executable(import_static_library
        include/cosimulation.h
        include/fmi3Functions.h
        include/fmi3FunctionTypes.h
        include/fmi3PlatformTypes.h
        include/model.h
        VanDerPol/config.h
        src/fmi3Functions.c
        VanDerPol/model.c
        src/cosimulation.c
        examples/import_static_library.c
    )
    set_target_properties (import_static_library PROPERTIES FOLDER examples)
    target_compile_definitions(import_static_library PRIVATE FMI_VERSION=${FMI_VERSION})
    target_include_directories(import_static_library PRIVATE include VanDerPol)
    set_target_properties(import_static_library PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY         temp
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
        RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
    )

    # import_shared_library
    add_executable(import_shared_library
        include/fmi3FunctionTypes.h
        include/fmi3PlatformTypes.h
        include/FMI.h
        include/FMI${FMI_VERSION}.h
        src/FMI.c
        src/FMI${FMI_VERSION}.c
        examples/import_shared_library.c
    )
    add_dependencies(import_shared_library VanDerPol)
    set_target_properties (import_shared_library PROPERTIES FOLDER examples)
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
    )
    add_dependencies(cs_early_return BouncingBall)
    set_target_properties(cs_early_return PROPERTIES FOLDER examples)
    target_compile_definitions(cs_early_return PRIVATE FMI_VERSION=${FMI_VERSION} DISABLE_PREFIX)
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
    )
    add_dependencies(cs_event_mode BouncingBall)
    set_target_properties(cs_event_mode PROPERTIES FOLDER examples)
    target_compile_definitions(cs_event_mode PRIVATE FMI_VERSION=${FMI_VERSION} DISABLE_PREFIX)
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
    )
    add_dependencies(cs_intermediate_update BouncingBall)
    set_target_properties(cs_intermediate_update PROPERTIES FOLDER examples)
    target_compile_definitions(cs_intermediate_update PRIVATE FMI_VERSION=${FMI_VERSION} DISABLE_PREFIX)
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
    )
    add_dependencies(jacobian VanDerPol)
    set_target_properties(jacobian PROPERTIES FOLDER examples)
    target_compile_definitions(jacobian PRIVATE FMI_VERSION=${FMI_VERSION} DISABLE_PREFIX)
    target_include_directories(jacobian PRIVATE include VanDerPol)
    target_link_libraries(jacobian ${LIBRARIES})
    set_target_properties(jacobian PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY         temp
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
        RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
    )

    # scs_synchronous
    add_executable (scs_synchronous
        ${EXAMPLE_SOURCES}
        Clocks/config.h
        examples/Clocks.c
        examples/scs_synchronous.c
    )
    add_dependencies(scs_synchronous Clocks)
    set_target_properties(scs_synchronous PROPERTIES FOLDER examples)
    target_compile_definitions(scs_synchronous PRIVATE FMI_VERSION=${FMI_VERSION})
    target_include_directories(scs_synchronous PRIVATE include Clocks)
    target_link_libraries(scs_synchronous ${LIBRARIES})
    set_target_properties(scs_synchronous PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY         temp
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
        RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
    )

    if (WIN32)
        add_executable (scs_threaded
            ${EXAMPLE_SOURCES}
            Clocks/config.h
            examples/scs_threaded.c
        )
        add_dependencies(scs_threaded Clocks)
        set_target_properties(scs_threaded PROPERTIES FOLDER examples)
        target_compile_definitions(scs_threaded PRIVATE FMI_VERSION=${FMI_VERSION})
        target_include_directories(scs_threaded PRIVATE include Clocks)
        target_link_libraries(scs_threaded ${LIBRARIES})
        set_target_properties(scs_threaded PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY         temp
            RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
            RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
        )
    endif ()

endif()

# Examples
set(MODEL_NAMES BouncingBall Dahlquist Feedthrough Stair VanDerPol)

if (${FMI_VERSION} EQUAL 1)
    if (${FMI_TYPE} STREQUAL CS)
        set(MODEL_NAMES ${MODEL_NAMES} Resource)
        set(INTERFACE_TYPES cs)
    else()
        set(INTERFACE_TYPES me)
    endif()
elseif(${FMI_VERSION} EQUAL 2)
    set(MODEL_NAMES ${MODEL_NAMES} Resource)
    set(INTERFACE_TYPES cs me)
else()
    set(MODEL_NAMES ${MODEL_NAMES} LinearTransform Resource)
    set(INTERFACE_TYPES cs me)
endif()

foreach (MODEL_NAME ${MODEL_NAMES})
    foreach (INTERFACE_TYPE ${INTERFACE_TYPES})
        set(TARGET_NAME ${MODEL_NAME}_${INTERFACE_TYPE})
        add_executable (${TARGET_NAME}
            ${EXAMPLE_SOURCES}
            ${MODEL_NAME}/config.h
            examples/simulate_fmi${FMI_VERSION}_${INTERFACE_TYPE}.c
            examples/${MODEL_NAME}.c
        )
        add_dependencies(${TARGET_NAME} ${MODEL_NAME})
        set_target_properties(${TARGET_NAME} PROPERTIES FOLDER examples)
        target_include_directories(${TARGET_NAME} PRIVATE include ${MODEL_NAME})
        target_compile_definitions(${TARGET_NAME} PRIVATE FMI_VERSION=${FMI_VERSION} DISABLE_PREFIX)
        target_link_libraries(${TARGET_NAME} ${LIBRARIES})
        set_target_properties(${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY         temp
            RUNTIME_OUTPUT_DIRECTORY_DEBUG   temp
            RUNTIME_OUTPUT_DIRECTORY_RELEASE temp
        )
    endforeach(INTERFACE_TYPE)
endforeach(MODEL_NAME)
