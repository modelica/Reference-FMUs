#include <QTemporaryDir>
#include <QProcess>
#include "BuildPlatformBinaryThread.h"


BuildPlatformBinaryThread::BuildPlatformBinaryThread(QObject *parent)
    : QThread{parent}
{}

static QString wslPath(const QString& path) {

    QString canonicalPath = path;

    canonicalPath = canonicalPath.replace('\\', '/');

    QProcess process;

    process.start("wsl", {"wslpath", "-a", canonicalPath});

    process.waitForFinished();

    QString p(process.readAllStandardOutput());

    return p.trimmed();
}

void BuildPlatformBinaryThread::run() {

    QString modelIdentifier;

    QTemporaryDir buildDirectory;

    buildDirectory.setAutoRemove(removeBuilDirectory);

    QFile::copy(":/build/CMakeLists.txt", buildDirectory.filePath("CMakeLists.txt"));

    if (modelDescription->fmiMajorVersion == 2) {
        QFile::copy(":/build/fmi2Functions.h", buildDirectory.filePath("fmi2Functions.h"));
        QFile::copy(":/build/fmi2FunctionTypes.h", buildDirectory.filePath("fmi2FunctionTypes.h"));
        QFile::copy(":/build/fmi2TypesPlatform.h", buildDirectory.filePath("fmi2TypesPlatform.h"));
    } else {
        QFile::copy(":/build/fmi3Functions.h", buildDirectory.filePath("fmi3Functions.h"));
        QFile::copy(":/build/fmi3FunctionTypes.h", buildDirectory.filePath("fmi3FunctionTypes.h"));
        QFile::copy(":/build/fmi3PlatformTypes.h", buildDirectory.filePath("fmi3PlatformTypes.h"));
    }

    size_t nSourceFiles;
    const char** sourceFiles;

    if (modelDescription->coSimulation) {
        modelIdentifier = modelDescription->coSimulation->modelIdentifier;
        nSourceFiles = modelDescription->coSimulation->nSourceFiles;
        sourceFiles = modelDescription->coSimulation->sourceFiles;
    } else {
        modelIdentifier = modelDescription->modelExchange->modelIdentifier;
        nSourceFiles = modelDescription->modelExchange->nSourceFiles;
        sourceFiles = modelDescription->modelExchange->sourceFiles;
    }

    QStringList definitions;

    if (modelDescription->fmiMajorVersion == 3) {
        definitions << "FMI3_OVERRIDE_FUNCTION_PREFIX";
    }

    if (buildDescription) {

        if (buildDescription->nBuildConfigurations > 1) {
            emit newMessage("Multiple Build Configurations are not supported.\n");
            return;
        }

        const FMIBuildConfiguration* buildConfiguration = buildDescription->buildConfigurations[0];

        if (buildConfiguration->nSourceFileSets > 1) {
            emit newMessage("Multiple Source File Sets are not supported.\n");
            return;
        }

        const FMISourceFileSet* sourceFileSet = buildConfiguration->sourceFileSets[0];

        nSourceFiles = sourceFileSet->nSourceFiles;
        sourceFiles = sourceFileSet->sourceFiles;

        for (size_t i = 0; i < sourceFileSet->nPreprocessorDefinitions; i++) {

            FMIPreprocessorDefinition* preprocessorDefinition = sourceFileSet->preprocessorDefinitions[i];

            QString definition = preprocessorDefinition->name;

            if (preprocessorDefinition->value) {
                definition += "=";
                definition += preprocessorDefinition->value;
            }

            definitions << definition;
        }
    }

    QString buildDirPath = compileWithWSL ? wslPath(buildDirectory.path()) : buildDirectory.path();
    QString unzipdirPath = compileWithWSL ? wslPath(unzipdir) : unzipdir;

    QStringList sources;

    for (size_t i = 0; i < nSourceFiles; i++) {
        sources << QDir(unzipdirPath).filePath("sources/" + QString(sourceFiles[i]));
    }

    QStringList includeDirectories = {
        buildDirPath,
        QDir(unzipdirPath).filePath("sources")
    };


    emit newMessage("Generating CMake project...\n");

    QString program;

    QProcess process;
    QStringList arguments;

    if (compileWithWSL) {
        program = "wsl";
        arguments << cmakeCommand;
    } else {
        program = cmakeCommand;
    }

    if (!cmakeGenerator.isEmpty()) {
        arguments << "-G" + cmakeGenerator;
    }

    arguments << "-S" + buildDirPath;
    arguments << "-B" + buildDirPath;
    arguments << "-DFMI_MAJOR_VERSION=" + QString::number(modelDescription->fmiMajorVersion);
    arguments << "-DMODEL_IDENTIFIER=" + modelIdentifier;
    arguments << "-DINCLUDE='" + includeDirectories.join(';') + "'";
    arguments << "-DDEFINITIONS='" + definitions.join(';') + "'";
    arguments << "-DSOURCES='" + sources.join(';') + "'";
    arguments << "-DUNZIPDIR='" + unzipdirPath + "'";

    process.start(program, arguments);

    bool success = process.waitForFinished();

    emit newMessage(process.readAllStandardOutput());
    emit newMessage(process.readAllStandardError());

    if (!success) {
        return;
    }

    emit newMessage("Building CMake project...\n");

    arguments.clear();

    if (compileWithWSL) {
        arguments << cmakeCommand;
    }

    arguments << "--build" << buildDirPath;
    arguments << "--target" << "install";
    arguments << "--config" << buildConfiguration;

    process.start(program, arguments);

    success = process.waitForFinished();

    emit newMessage(process.readAllStandardOutput());
    emit newMessage(process.readAllStandardError());
}
