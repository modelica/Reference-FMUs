#include <errno.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <process.h>
#include <strsafe.h>

#include "FMIZip.h"


const char* FMICreateTemporaryDirectory() {
    
    const char* tempfile = _tempnam(NULL, NULL);

    if (!tempfile) {
        return NULL;
    }

    BOOL res = CreateDirectoryA(tempfile, 0);

    if (!PathFileExistsA(tempfile)) {
        free((void*)tempfile);
        return NULL;
    }

    return tempfile;
}

int FMIPathAppend(char* path, const char* more) {

    return PathAppendA(path, "modelDescription.xml");
}

int FMIExtractArchive(const char* filename, const char* unzipdir) {


#ifdef _WIN32

    char command[2048] = "";

    snprintf(command, 2048, "xcopy \"%s\" \"%s\"", filename, unzipdir);

    int status = system(command);

    snprintf(command, 2048, "ren \"%s\\*.fmu\" *.zip", unzipdir);

    status = system(command);

    snprintf(command, 2048, "powershell -ExecutionPolicy Bypass -command \"Expand-Archive -Force '%s\\*.zip' '%s'\"", unzipdir, unzipdir);

    status = system(command);

    snprintf(command, 2048, "del /f \"%s\\*.zip\"", unzipdir);

    status = system(command);

    return status;

#else

    // TODO

#endif

    return 0;
}

int FMIRemoveDirectory(const char* path) {

    char command[4096];

    sprintf(command, "rmdir \"%s\" /s /q", path);

    return system(command);
}
