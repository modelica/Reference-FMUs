cmake_minimum_required (VERSION 3.17)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project (FMUSimulator)

set(CONDA_DIR "" CACHE STRING "Conda directory")

add_executable(fmusim
  ../include/FMI.h
  ../src/FMI.c
  ../include/FMI3.h
  ../src/FMI3.c
  FMIModelDescription.h
  FMIModelDescription.c
  FMISimulationResult.h
  FMISimulationResult.c
  FMIZip.h
  FMIZip.c
  fmusim.c
  fmusim_fmi3_cs.h
  fmusim_fmi3_cs.c
  fmusim_fmi3_me.h
  fmusim_fmi3_me.c
)

target_include_directories(fmusim PRIVATE
  .
  ../include
  ${CONDA_DIR}/include
  ${CONDA_DIR}/Library/include
  ${CONDA_DIR}/Library/include/libxml2
)

target_link_libraries(fmusim
  ${CONDA_DIR}/Library/lib/libxml2.lib
  ${CONDA_DIR}/Library/lib/zip.lib
)

file(GLOB API_MS_DLLS ${CONDA_DIR}/Library/bin/api-ms-win-*.dll)

install(TARGETS fmusim RUNTIME DESTINATION .)

install(FILES
    ${CONDA_DIR}/Library/bin/libxml2.dll
    ${CONDA_DIR}/Library/bin/iconv.dll
    ${CONDA_DIR}/Library/bin/charset.dll
    ${CONDA_DIR}/Library/bin/zlib.dll
    ${CONDA_DIR}/Library/bin/zip.dll
    ${CONDA_DIR}/Library/bin/zip.dll
    # ${CONDA_DIR}/Library/bin/advapi32.dll
    ${CONDA_DIR}/Library/bin/libbz2.dll
    # ${CONDA_DIR}/Library/bin/bcrypt.dll
    ${CONDA_DIR}/Library/bin/vcruntime140.dll
    ${API_MS_DLLS}
    DESTINATION .)

install(FILES
    ../schema/fmi2/fmi2Annotation.xsd
    ../schema/fmi2/fmi2Annotation.xsd
    ../schema/fmi2/fmi2AttributeGroups.xsd
    ../schema/fmi2/fmi2ModelDescription.xsd
    ../schema/fmi2/fmi2ScalarVariable.xsd
    ../schema/fmi2/fmi2Type.xsd
    ../schema/fmi2/fmi2Unit.xsd
    ../schema/fmi2/fmi2VariableDependency.xsd
    DESTINATION ./schema/fmi2)

install(FILES
    ../schema/fmi3/fmi3Annotation.xsd
    ../schema/fmi3/fmi3AttributeGroups.xsd
    ../schema/fmi3/fmi3BuildDescription.xsd
    ../schema/fmi3/fmi3InterfaceType.xsd
    ../schema/fmi3/fmi3ModelDescription.xsd
    ../schema/fmi3/fmi3Terminal.xsd
    ../schema/fmi3/fmi3TerminalsAndIcons.xsd
    ../schema/fmi3/fmi3Type.xsd
    ../schema/fmi3/fmi3Unit.xsd
    ../schema/fmi3/fmi3Variable.xsd
    ../schema/fmi3/fmi3VariableDependency.xsd
    DESTINATION ./schema/fmi3)
