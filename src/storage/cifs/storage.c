/***********************************************************************************************************************************
CIFS Storage
***********************************************************************************************************************************/
#include "build.auto.h"

#include "common/debug.h"
#include "common/log.h"
#include "common/regExp.h"
#include "storage/cifs/storage.h"
#include "storage/posix/storage.intern.h"

/**********************************************************************************************************************************/
FN_EXTERN Storage *
storageCifsNew(const Path *const path, const mode_t modeFile, const mode_t modePath, const bool write)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(PATH, path);
        FUNCTION_LOG_PARAM(MODE, modeFile);
        FUNCTION_LOG_PARAM(MODE, modePath);
        FUNCTION_LOG_PARAM(BOOL, write);
    FUNCTION_LOG_END();

    FUNCTION_LOG_RETURN(
        STORAGE, storagePosixNewInternal(STORAGE_CIFS_TYPE, path, modeFile, modePath, write, false));
}
