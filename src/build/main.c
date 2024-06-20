/***********************************************************************************************************************************
Code Builder
***********************************************************************************************************************************/
#include "build.auto.h"

#include <unistd.h>

#include "common/log.h"
#include "storage/posix/storage.h"

#include "build/config/parse.h"
#include "build/config/render.h"
#include "build/error/parse.h"
#include "build/error/render.h"
#include "build/help/parse.h"
#include "build/help/render.h"
#include "build/postgres/parse.h"
#include "build/postgres/render.h"

static Path *getCurrentWorkDir(void)
{
    char currentWorkDir[1024];
    THROW_ON_SYS_ERROR(getcwd(currentWorkDir, sizeof(currentWorkDir)) == NULL, FormatError, "unable to get cwd");

    return pathNewZ(currentWorkDir);
}

int
main(const int argListSize, const char *const argList[])
{
    // Set stack trace and mem context error cleanup handlers
    static const ErrorHandlerFunction errorHandlerList[] = {stackTraceClean, memContextClean};
    errorHandlerSet(errorHandlerList, LENGTH_OF(errorHandlerList));

    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(INT, argListSize);
        FUNCTION_LOG_PARAM(CHARPY, argList);
    FUNCTION_LOG_END();

    int result = 0;

    TRY_BEGIN()
    {
        // Check parameters
        CHECK(ParamInvalidError, argListSize >= 2 && argListSize <= 4, "only one to three parameters allowed");

        // Initialize logging
        logInit(logLevelWarn, logLevelError, logLevelOff, false, 0, 1, false);

        // Get current working directory
        const Path *currentWorkDir = getCurrentWorkDir();
        const Path *repoPath;
        const Path *buildPath;

        // Get repo path (cwd if it was not passed)
        if (argListSize >= 3)
        {
            Path *const pathArg = pathNewZ(argList[2]);

            if (pathIsAbsolute(pathArg))
            {
                repoPath = pathGetParent(pathArg);
                pathFree(pathArg);
            }
            else
                repoPath = pathMakeAbsolute(pathArg, currentWorkDir);
        }
        else
            repoPath = pathGetParent(currentWorkDir);

        // If the build path was specified
        if (argListSize >= 4)
        {
            Path *const pathArg = pathNewZ(argList[3]);

            if (pathIsAbsolute(pathArg))
                buildPath = pathArg;
            else
                buildPath = pathMakeAbsolute(pathArg, currentWorkDir);
        }
        else
            buildPath = pathDup(repoPath);

        // Repo and build storage
        const Storage *const storageRepo = storagePosixNewP(repoPath);
        const Storage *const storageBuild = storagePosixNewP(buildPath, .write = true);

        // Config
        if (strEqZ(STRDEF("config"), argList[1]))
            bldCfgRender(storageBuild, bldCfgParse(storageRepo), true);

        // Error
        if (strEqZ(STRDEF("error"), argList[1]))
            bldErrRender(storageBuild, bldErrParse(storageRepo));

        // Help
        if (strEqZ(STRDEF("help"), argList[1]))
        {
            const BldCfg bldCfg = bldCfgParse(storageRepo);
            bldHlpRender(storageBuild, bldCfg, bldHlpParse(storageRepo, bldCfg, false));
        }

        // PostgreSQL
        if (strEqZ(STRDEF("postgres"), argList[1]))
            bldPgRender(storageBuild, bldPgParse(storageRepo));

        if (strEqZ(STRDEF("postgres-version"), argList[1]))
            bldPgVersionRender(storageBuild, bldPgParse(storageRepo));
    }
    CATCH_FATAL()
    {
        LOG_FMT(
            errorCode() == errorTypeCode(&AssertError) ? logLevelAssert : logLevelError, errorCode(), "%s\n%s", errorMessage(),
            errorStackTrace());

        result = errorCode();
    }
    TRY_END();

    FUNCTION_LOG_RETURN(INT, result);
}
