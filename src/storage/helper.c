/***********************************************************************************************************************************
Storage Helper
***********************************************************************************************************************************/
#include "build.auto.h"

#include <string.h>

#include "common/debug.h"
#include "common/io/io.h"
#include "common/memContext.h"
#include "common/regExp.h"
#include "config/config.h"
#include "protocol/helper.h"
#include "storage/helper.h"
#include "storage/posix/storage.h"
#include "storage/remote/storage.h"
#include "storage/vfs/storage.h"

/***********************************************************************************************************************************
Storage path constants
***********************************************************************************************************************************/
STRING_EXTERN(STORAGE_SPOOL_ARCHIVE_STR,                            STORAGE_SPOOL_ARCHIVE);
STRING_EXTERN(STORAGE_SPOOL_ARCHIVE_IN_STR,                         STORAGE_SPOOL_ARCHIVE_IN);
STRING_EXTERN(STORAGE_SPOOL_ARCHIVE_OUT_STR,                        STORAGE_SPOOL_ARCHIVE_OUT);

STRING_EXTERN(STORAGE_REPO_ARCHIVE_STR,                             STORAGE_REPO_ARCHIVE);
STRING_EXTERN(STORAGE_REPO_BACKUP_STR,                              STORAGE_REPO_BACKUP);

STRING_EXTERN(STORAGE_PATH_ARCHIVE_STR,                             STORAGE_PATH_ARCHIVE);
STRING_EXTERN(STORAGE_PATH_BACKUP_STR,                              STORAGE_PATH_BACKUP);

/***********************************************************************************************************************************
Error message when writable storage is requested in dry-run mode
***********************************************************************************************************************************/
#define WRITABLE_WHILE_DRYRUN                                                                                                      \
    "unable to get writable storage in dry-run mode or before dry-run is initialized"

/***********************************************************************************************************************************
Local variables
***********************************************************************************************************************************/
static struct StorageHelperLocal
{
    MemContext *memContext;                                         // Mem context for storage helper

    const StorageHelper *helperList;                                // List of helpers to create storage

    Storage *storageLocal;                                          // Local read-only storage
    Storage *storageLocalWrite;                                     // Local write storage
    Storage **storagePg;                                            // PostgreSQL read-only storage
    Storage **storagePgWrite;                                       // PostgreSQL write storage
    Storage **storageRepo;                                          // Repository read-only storage
    Storage **storageRepoWrite;                                     // Repository write storage
    Storage *storageSpool;                                          // Spool read-only storage
    Storage *storageSpoolWrite;                                     // Spool write storage

    String *stanza;                                                 // Stanza for storage
    bool stanzaInit;                                                // Has the stanza been initialized?
    bool dryRunInit;                                                // Has dryRun been initialized? If not disallow writes.
    bool dryRun;                                                    // Disallow writes in dry-run mode.
    RegExp *walRegExp;                                              // Regular expression for identifying wal files
} storageHelper;

/***********************************************************************************************************************************
Create the storage helper memory context
***********************************************************************************************************************************/
static void
storageHelperContextInit(void)
{
    FUNCTION_TEST_VOID();

    if (storageHelper.memContext == NULL)
    {
        MEM_CONTEXT_BEGIN(memContextTop())
        {
            MEM_CONTEXT_NEW_BEGIN(StorageHelper, .childQty = MEM_CONTEXT_QTY_MAX, .allocQty = MEM_CONTEXT_QTY_MAX)
            {
                storageHelper.memContext = MEM_CONTEXT_NEW();
            }
            MEM_CONTEXT_NEW_END();
        }
        MEM_CONTEXT_END();
    }

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN void
storageHelperInit(const StorageHelper *const helperList)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM_P(VOID, helperList);
    FUNCTION_TEST_END();

    storageHelper.helperList = helperList;

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN void
storageHelperDryRunInit(const bool dryRun)
{
    FUNCTION_TEST_VOID();

    storageHelper.dryRunInit = true;
    storageHelper.dryRun = dryRun;

    FUNCTION_TEST_RETURN_VOID();
}

/***********************************************************************************************************************************
Initialize the stanza and error if it changes
***********************************************************************************************************************************/
static void
storageHelperStanzaInit(const bool stanzaRequired)
{
    FUNCTION_TEST_VOID();

    // If the stanza is NULL and the storage has not already been initialized then initialize the stanza
    if (!storageHelper.stanzaInit)
    {
        if (stanzaRequired && cfgOptionStrNull(cfgOptStanza) == NULL)
            THROW(AssertError, "stanza cannot be NULL for this storage object");

        MEM_CONTEXT_BEGIN(storageHelper.memContext)
        {
            storageHelper.stanza = strDup(cfgOptionStrNull(cfgOptStanza));
            storageHelper.stanzaInit = true;
        }
        MEM_CONTEXT_END();
    }

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN const Storage *
storageLocal(void)
{
    FUNCTION_TEST_VOID();

    if (storageHelper.storageLocal == NULL)
    {
        storageHelperContextInit();

        MEM_CONTEXT_BEGIN(storageHelper.memContext)
        {
            storageHelper.storageLocal = storagePosixNewStrP(FSLASH_STR);
        }
        MEM_CONTEXT_END();
    }

    FUNCTION_TEST_RETURN_CONST(STORAGE, storageHelper.storageLocal);
}

FN_EXTERN const Storage *
storageLocalWrite(void)
{
    FUNCTION_TEST_VOID();

    if (storageHelper.storageLocalWrite == NULL)
    {
        storageHelperContextInit();

        MEM_CONTEXT_BEGIN(storageHelper.memContext)
        {
            storageHelper.storageLocalWrite = storagePosixNewStrP(FSLASH_STR, .write = true);
        }
        MEM_CONTEXT_END();
    }

    FUNCTION_TEST_RETURN_CONST(STORAGE, storageHelper.storageLocalWrite);
}

/***********************************************************************************************************************************
Get pg storage for the specified host id
***********************************************************************************************************************************/
static Storage *
storagePgGet(const unsigned int pgIdx, const bool write)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(UINT, pgIdx);
        FUNCTION_TEST_PARAM(BOOL, write);
    FUNCTION_TEST_END();

    Storage *result;

    // Use remote storage
    if (!pgIsLocal(pgIdx))
    {
        result = storageRemoteNew(
            STORAGE_MODE_FILE_DEFAULT, STORAGE_MODE_PATH_DEFAULT, write,
            protocolRemoteGet(protocolStorageTypePg, pgIdx), cfgOptionUInt(cfgOptCompressLevelNetwork));
    }
    // Use VFS storage
    else
    {
        MEM_CONTEXT_TEMP_BEGIN();
        {
            StorageVfsMountPointList *mountPoints = storageVfsMountPointListNew();

            StorageVfsMountPoint dataMountPoint =
            {
                .storage = storagePosixNewStrP(cfgOptionIdxStr(cfgOptPgPath, pgIdx), .write = write),
                .expression = STR(STORAGE_PG_DATA),
                .virtualFolder = STR("6c910630-d9c1-43f4-9702-bb4bdb5d0173")
            };

            storageVfsMountPointListAdd(mountPoints, &dataMountPoint);

            MEM_CONTEXT_PRIOR_BEGIN();
            {
                result = storageVfsNewP(mountPoints);
            }
            MEM_CONTEXT_PRIOR_END();
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_TEST_RETURN(STORAGE, result);
}

/**********************************************************************************************************************************/
FN_EXTERN const Storage *
storagePgIdx(const unsigned int pgIdx)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(UINT, pgIdx);
    FUNCTION_TEST_END();

    if (storageHelper.storagePg == NULL || storageHelper.storagePg[pgIdx] == NULL)
    {
        storageHelperContextInit();

        MEM_CONTEXT_BEGIN(storageHelper.memContext)
        {
            if (storageHelper.storagePg == NULL)
                storageHelper.storagePg = memNewPtrArray(cfgOptionGroupIdxTotal(cfgOptGrpPg));

            storageHelper.storagePg[pgIdx] = storagePgGet(pgIdx, false);
        }
        MEM_CONTEXT_END();
    }

    FUNCTION_TEST_RETURN_CONST(STORAGE, storageHelper.storagePg[pgIdx]);
}

FN_EXTERN const Storage *
storagePg(void)
{
    FUNCTION_TEST_VOID();
    FUNCTION_TEST_RETURN_CONST(STORAGE, storagePgIdx(cfgOptionGroupIdxDefault(cfgOptGrpPg)));
}

FN_EXTERN const Storage *
storagePgIdxWrite(const unsigned int pgIdx)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(UINT, pgIdx);
    FUNCTION_TEST_END();

    // Writes not allowed in dry-run mode
    if (!storageHelper.dryRunInit || storageHelper.dryRun)
        THROW(AssertError, WRITABLE_WHILE_DRYRUN);

    if (storageHelper.storagePgWrite == NULL || storageHelper.storagePgWrite[pgIdx] == NULL)
    {
        storageHelperContextInit();

        MEM_CONTEXT_BEGIN(storageHelper.memContext)
        {
            if (storageHelper.storagePgWrite == NULL)
                storageHelper.storagePgWrite = memNewPtrArray(cfgOptionGroupIdxTotal(cfgOptGrpPg));

            storageHelper.storagePgWrite[pgIdx] = storagePgGet(pgIdx, true);
        }
        MEM_CONTEXT_END();
    }

    FUNCTION_TEST_RETURN_CONST(STORAGE, storageHelper.storagePgWrite[pgIdx]);
}

FN_EXTERN const Storage *
storagePgWrite(void)
{
    FUNCTION_TEST_VOID();
    FUNCTION_TEST_RETURN_CONST(STORAGE, storagePgIdxWrite(cfgOptionGroupIdxDefault(cfgOptGrpPg)));
}

/***********************************************************************************************************************************
Create the WAL regular expression
***********************************************************************************************************************************/
static void
storageHelperRepoInit(void)
{
    FUNCTION_TEST_VOID();

    if (storageHelper.walRegExp == NULL)
    {
        MEM_CONTEXT_BEGIN(memContextTop())
        {
            storageHelper.walRegExp = regExpNew(STRDEF("^[0-F]{24}"));
        }
        MEM_CONTEXT_END();
    }

    FUNCTION_TEST_RETURN_VOID();
}

/***********************************************************************************************************************************
Construct a repo path from a path expression
***********************************************************************************************************************************/
static Path *
storageRepoPathExpression(const Path *const path)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, path);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);

    Path *result = NULL;
    const String *const expression = pathGetRoot(path);

    if (strEq(expression, STORAGE_REPO_ARCHIVE_STR))
    {
        if (storageHelper.stanza != NULL)
            result = pathResolveExpressionFmt(path, STORAGE_PATH_ARCHIVE "/%s", strZ(storageHelper.stanza));
        else
            result = pathResolveExpressionStr(path, STORAGE_PATH_ARCHIVE_STR);

        // Determine if the path expression is a WAL path (3 components: the root expression, a directory and the wal name)
        if (pathGetComponentCount(path) == 3 && regExpMatch(storageHelper.walRegExp, pathGetComponent(path, 2)))
        {
            const String *const walName = pathGetComponent(path, 2);

            pathAppendComponent(result, DOTDOT_STR);
            pathAppendComponentZN(result, strZ(walName), 16);
            pathAppendComponent(result, walName);
        }
    }
    else if (strEq(expression, STORAGE_REPO_BACKUP_STR))
    {
        if (storageHelper.stanza != NULL)
            result = pathResolveExpressionFmt(path, STORAGE_PATH_BACKUP "/%s", strZ(storageHelper.stanza));
        else
            result = pathResolveExpressionZ(path, STORAGE_PATH_BACKUP);
    }
    else
        THROW_FMT(AssertError, "invalid expression '%s'", strZ(expression));

    ASSERT(result != NULL);

    FUNCTION_TEST_RETURN(STRING, result);
}

/***********************************************************************************************************************************
Get the repo storage
***********************************************************************************************************************************/
static Storage *
storageRepoGet(const unsigned int repoIdx, const bool write)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(UINT, repoIdx);
        FUNCTION_TEST_PARAM(BOOL, write);
    FUNCTION_TEST_END();

    Storage *result = NULL;

    // Use remote storage
    if (!repoIsLocal(repoIdx))
    {
        result = storageRemoteNew(
            STORAGE_MODE_FILE_DEFAULT, STORAGE_MODE_PATH_DEFAULT, write,
            protocolRemoteGet(protocolStorageTypeRepo, repoIdx), cfgOptionUInt(cfgOptCompressLevelNetwork));
    }
    // Use local storage
    else
    {
        // Search for the helper
        const StringId type = cfgOptionIdxStrId(cfgOptRepoType, repoIdx);

        if (storageHelper.helperList != NULL)
        {
            for (const StorageHelper *helper = storageHelper.helperList; helper->type != 0; helper++)
            {
                if (helper->type == type)
                {
                    result = helper->helper(repoIdx, write);
                    break;
                }
            }
        }

        // If no helper was found it try Posix
        if (result == NULL)
        {
            CHECK(AssertError, type == STORAGE_POSIX_TYPE, "invalid storage type");

            result = storagePosixNewStrP(
                cfgOptionIdxStr(cfgOptRepoPath, repoIdx), .write = write);
        }

        MEM_CONTEXT_TEMP_BEGIN();
        {
            StorageVfsMountPointList *mountPoints = storageVfsMountPointListNew();

            StorageVfsMountPoint mountPoint =
            {
                .storage = result,
                .virtualFolder = STR("ae2370d3-2df1-44ed-881b-ff3fa167adfb"),
                .expression = STORAGE_REPO_ARCHIVE_STR,
                .callback = storageRepoPathExpression
            };

            storageVfsMountPointListAdd(mountPoints, &mountPoint);

            mountPoint.expression = strDup(STORAGE_REPO_BACKUP_STR);

            storageVfsMountPointListAdd(mountPoints, &mountPoint);

            MEM_CONTEXT_PRIOR_BEGIN();
            {
                result = storageVfsNewP(mountPoints);
            }
            MEM_CONTEXT_PRIOR_END();
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_TEST_RETURN(STORAGE, result);
}

/**********************************************************************************************************************************/
FN_EXTERN const Storage *
storageRepoIdx(const unsigned int repoIdx)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(UINT, repoIdx);
    FUNCTION_TEST_END();

    if (storageHelper.storageRepo == NULL)
    {
        storageHelperContextInit();
        storageHelperStanzaInit(false);
        storageHelperRepoInit();

        MEM_CONTEXT_BEGIN(storageHelper.memContext)
        {
            storageHelper.storageRepo = memNewPtrArray(cfgOptionGroupIdxTotal(cfgOptGrpRepo));
        }
        MEM_CONTEXT_END();
    }

    if (storageHelper.storageRepo[repoIdx] == NULL)
    {
        MEM_CONTEXT_BEGIN(storageHelper.memContext)
        {
            storageHelper.storageRepo[repoIdx] = storageRepoGet(repoIdx, false);
        }
        MEM_CONTEXT_END();
    }

    FUNCTION_TEST_RETURN_CONST(STORAGE, storageHelper.storageRepo[repoIdx]);
}

FN_EXTERN const Storage *
storageRepo(void)
{
    FUNCTION_TEST_VOID();
    FUNCTION_TEST_RETURN_CONST(STORAGE, storageRepoIdx(cfgOptionGroupIdxDefault(cfgOptGrpRepo)));
}

FN_EXTERN const Storage *
storageRepoIdxWrite(const unsigned int repoIdx)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(UINT, repoIdx);
    FUNCTION_TEST_END();

    // Writes not allowed in dry-run mode
    if (!storageHelper.dryRunInit || storageHelper.dryRun)
        THROW(AssertError, WRITABLE_WHILE_DRYRUN);

    if (storageHelper.storageRepoWrite == NULL)
    {
        storageHelperContextInit();
        storageHelperStanzaInit(false);
        storageHelperRepoInit();

        MEM_CONTEXT_BEGIN(storageHelper.memContext)
        {
            storageHelper.storageRepoWrite = memNewPtrArray(cfgOptionGroupIdxTotal(cfgOptGrpRepo));
        }
        MEM_CONTEXT_END();
    }

    if (storageHelper.storageRepoWrite[repoIdx] == NULL)
    {
        MEM_CONTEXT_BEGIN(storageHelper.memContext)
        {
            storageHelper.storageRepoWrite[repoIdx] = storageRepoGet(repoIdx, true);
        }
        MEM_CONTEXT_END();
    }

    FUNCTION_TEST_RETURN_CONST(STORAGE, storageHelper.storageRepoWrite[repoIdx]);
}

FN_EXTERN const Storage *
storageRepoWrite(void)
{
    FUNCTION_TEST_VOID();
    FUNCTION_TEST_RETURN_CONST(STORAGE, storageRepoIdxWrite(cfgOptionGroupIdxDefault(cfgOptGrpRepo)));
}

/***********************************************************************************************************************************
Spool storage path expression
***********************************************************************************************************************************/
static Path *
storageSpoolPathExpression(const Path *const path)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, path);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);
    ASSERT(storageHelper.stanza != NULL);

    Path *result = NULL;
    const String *const expression = pathGetRoot(path);

    if (strEqZ(expression, STORAGE_SPOOL_ARCHIVE))
        result = pathResolveExpressionFmt(path, STORAGE_PATH_ARCHIVE "/%s", strZ(storageHelper.stanza));
    else if (strEqZ(expression, STORAGE_SPOOL_ARCHIVE_IN))
        result = pathResolveExpressionFmt(path, STORAGE_PATH_ARCHIVE "/%s/in", strZ(storageHelper.stanza));
    else if (strEqZ(expression, STORAGE_SPOOL_ARCHIVE_OUT))
        result = pathResolveExpressionFmt(path, STORAGE_PATH_ARCHIVE "/%s/out", strZ(storageHelper.stanza));
    else
        THROW_FMT(AssertError, "invalid expression '%s'", strZ(expression));

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN const Storage *
storageSpool(void)
{
    FUNCTION_TEST_VOID();

    if (storageHelper.storageSpool == NULL)
    {
        storageHelperContextInit();
        storageHelperStanzaInit(true);

        MEM_CONTEXT_BEGIN(storageHelper.memContext)
        {
            storageHelper.storageSpool = storagePosixNewStrP(cfgOptionStr(cfgOptSpoolPath));
        }
        MEM_CONTEXT_END();
    }

    FUNCTION_TEST_RETURN_CONST(STORAGE, storageHelper.storageSpool);
}

FN_EXTERN const Storage *
storageSpoolWrite(void)
{
    FUNCTION_TEST_VOID();

    // Writes not allowed in dry-run mode
    if (!storageHelper.dryRunInit || storageHelper.dryRun)
        THROW(AssertError, WRITABLE_WHILE_DRYRUN);

    if (storageHelper.storageSpoolWrite == NULL)
    {
        storageHelperContextInit();
        storageHelperStanzaInit(true);

        MEM_CONTEXT_BEGIN(storageHelper.memContext)
        {
            storageHelper.storageSpoolWrite = storagePosixNewStrP(cfgOptionStr(cfgOptSpoolPath), .write = true);
        }
        MEM_CONTEXT_END();
    }

    FUNCTION_TEST_RETURN_CONST(STORAGE, storageHelper.storageSpoolWrite);
}

/**********************************************************************************************************************************/
FN_EXTERN void
storageHelperFree(void)
{
    FUNCTION_TEST_VOID();

    if (storageHelper.memContext != NULL)
        memContextFree(storageHelper.memContext);

    storageHelper = (struct StorageHelperLocal){.memContext = NULL, .helperList = storageHelper.helperList};

    FUNCTION_TEST_RETURN_VOID();
}
