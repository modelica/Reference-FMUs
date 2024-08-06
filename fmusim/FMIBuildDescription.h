#pragma once

#include <stddef.h>
#include <stdbool.h>


typedef struct {

    const char* name;
    const char* value;
    bool optional;
    const char* description;

} FMIPreprocessorDefinition;

typedef struct {

    const char* name;
    const char* language;
    const char* compiler;
    const char* compilerOptions;

    size_t nPreprocessorDefinitions;
    FMIPreprocessorDefinition** preprocessorDefinitions;

    size_t nSourceFiles;
    const char** sourceFiles;

    size_t nIncludeDirectories;
    const char** includeDirectories;

} FMISourceFileSet;

typedef struct {

    const char* modelIdentifier;

    size_t nSourceFileSets;
    FMISourceFileSet** sourceFileSets;

} FMIBuildConfiguration;

typedef struct {

    size_t nBuildConfigurations;
    FMIBuildConfiguration** buildConfigurations;

} FMIBuildDescription;


FMIBuildDescription* FMIReadBuildDescription(const char* filename);

void FMIFreeBuildDescription(FMIBuildDescription* buildDescription);
