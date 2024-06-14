/***********************************************************************************************************************************
CIFS Storage Helper
***********************************************************************************************************************************/
#include "build.auto.h"

#include "common/debug.h"
#include "common/log.h"
#include "config/config.h"
#include "storage/cifs/helper.h"

/**********************************************************************************************************************************/
FN_EXTERN Storage *
storageCifsHelper(const unsigned int repoIdx, const bool write, StoragePathExpressionCallback pathExpressionCallback)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(UINT, repoIdx);
        FUNCTION_LOG_PARAM(BOOL, write);
        FUNCTION_LOG_PARAM_P(VOID, pathExpressionCallback);
    FUNCTION_LOG_END();

    ASSERT(cfgOptionIdxStrId(cfgOptRepoType, repoIdx) == STORAGE_CIFS_TYPE);

    Storage *result = NULL;

    MEM_CONTEXT_TEMP_BEGIN();
    {
        const Path *const repoPath = pathNew(cfgOptionIdxStr(cfgOptRepoPath, repoIdx));

        MEM_CONTEXT_PRIOR_BEGIN();
        {
            result = storageCifsNew(
                    repoPath, STORAGE_MODE_FILE_DEFAULT, STORAGE_MODE_PATH_DEFAULT, write,
                    pathExpressionCallback);
        }
        MEM_CONTEXT_PRIOR_END();
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(STORAGE, result);
}
