cmake_minimum_required (VERSION 3.17)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project (FMUSimulator)

add_executable(fmusim
  ../include/FMI.h
  ../src/FMI.c
  ../include/FMI2.h
  ../src/FMI2.c
  ../include/FMI3.h
  ../src/FMI3.c
  csv.h
  csv.c
  FMIModelDescription.h
  FMIModelDescription.c
  FMISimulationResult.h
  FMISimulationResult.c
  FMIZip.h
  FMIZip.c
  fmusim.c
  fmusim_fmi2.h
  fmusim_fmi2.c
  fmusim_fmi2_cs.h
  fmusim_fmi2_cs.c
  fmusim_fmi2_me.h
  fmusim_fmi2_me.c
  fmusim_fmi3.h
  fmusim_fmi3.c
  fmusim_fmi3_cs.h
  fmusim_fmi3_cs.c
  fmusim_fmi3_me.h
  fmusim_fmi3_me.c
  fmusim_input.h
  fmusim_input.c
)

target_include_directories(fmusim PRIVATE
  .
  ../include
  E:/Development/Reference-FMUs/ThirdParty/libxml2/win64/install/include/libxml2
)

add_compile_definitions(LIBXML_STATIC)

target_link_libraries(fmusim
  E:/Development/Reference-FMUs/ThirdParty/libxml2/win64/install/lib/libxml2s.lib
  wsock32
  ws2_32
)

install(TARGETS fmusim RUNTIME DESTINATION .)

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
