/***********************************************************************************************************************************
GCS Storage Helper
***********************************************************************************************************************************/
#include "build.auto.h"

#include "common/debug.h"
#include "common/io/io.h"
#include "common/log.h"
#include "config/config.h"
#include "storage/gcs/helper.h"

/**********************************************************************************************************************************/
FN_EXTERN Storage *
storageGcsHelper(const unsigned int repoIdx, const bool write)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(UINT, repoIdx);
        FUNCTION_LOG_PARAM(BOOL, write);
    FUNCTION_LOG_END();

    ASSERT(cfgOptionIdxStrId(cfgOptRepoType, repoIdx) == STORAGE_GCS_TYPE);

    Storage *result = NULL;

    MEM_CONTEXT_TEMP_BEGIN();
    {
        const Path *const repoPath = pathNew(cfgOptionIdxStr(cfgOptRepoPath, repoIdx));

        MEM_CONTEXT_PRIOR_BEGIN();
        {
            result = storageGcsNew(
                repoPath, write, cfgOptionIdxStr(cfgOptRepoGcsBucket, repoIdx),
                (StorageGcsKeyType)cfgOptionIdxStrId(cfgOptRepoGcsKeyType, repoIdx), cfgOptionIdxStrNull(cfgOptRepoGcsKey, repoIdx),
                (size_t)cfgOptionIdxUInt64(cfgOptRepoStorageUploadChunkSize, repoIdx), cfgOptionIdxKvNull(cfgOptRepoStorageTag, repoIdx),
                cfgOptionIdxStr(cfgOptRepoGcsEndpoint, repoIdx), ioTimeoutMs(), cfgOptionIdxBool(cfgOptRepoStorageVerifyTls, repoIdx),
                cfgOptionIdxStrNull(cfgOptRepoStorageCaFile, repoIdx), cfgOptionIdxStrNull(cfgOptRepoStorageCaPath, repoIdx));
        }
        MEM_CONTEXT_PRIOR_END();
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(STORAGE, result);
}
