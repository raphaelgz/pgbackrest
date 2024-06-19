/***********************************************************************************************************************************
Storage Interface
***********************************************************************************************************************************/
#include "build.auto.h"

#include <stdio.h>
#include <string.h>

#include "common/debug.h"
#include "common/io/io.h"
#include "common/log.h"
#include "common/memContext.h"
#include "common/path.h"
#include "common/regExp.h"
#include "common/type/list.h"
#include "common/wait.h"
#include "storage/storage.h"

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
struct Storage
{
    StoragePub pub;                                                 // Publicly accessible variables
    Path *path;
    mode_t modeFile;
    mode_t modePath;
    bool write;
    StoragePathExpressionCallback *pathExpressionFunction;
};

/**********************************************************************************************************************************/
FN_EXTERN Storage *
storageNew(
    const StringId type, const Path *const path, const mode_t modeFile, const mode_t modePath, const bool write,
    StoragePathExpressionCallback pathExpressionFunction, void *const driver, const StorageInterface interface)
{
    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STRING_ID, type);
        FUNCTION_LOG_PARAM(PATH, path);
        FUNCTION_LOG_PARAM(MODE, modeFile);
        FUNCTION_LOG_PARAM(MODE, modePath);
        FUNCTION_LOG_PARAM(BOOL, write);
        FUNCTION_LOG_PARAM(FUNCTIONP, pathExpressionFunction);
        FUNCTION_LOG_PARAM_P(VOID, driver);
        FUNCTION_LOG_PARAM(STORAGE_INTERFACE, interface);
    FUNCTION_LOG_END();

    FUNCTION_AUDIT_HELPER();

    ASSERT(type != 0);
    ASSERT(path != NULL && pathIsAbsolute(path));
    ASSERT(driver != NULL);
    ASSERT(interface.info != NULL);
    ASSERT(interface.list != NULL);
    ASSERT(interface.newRead != NULL);
    ASSERT(interface.newWrite != NULL);
    ASSERT(interface.pathRemove != NULL);
    ASSERT(interface.remove != NULL);

    OBJ_NEW_BEGIN(Storage, .childQty = MEM_CONTEXT_QTY_MAX)
    {
        *this = (Storage)
        {
            .pub =
            {
                .type = type,
                .driver = objMoveToInterface(driver, this, memContextPrior()),
                .interface = interface,
            },
            .path = pathDup(path),
            .modeFile = modeFile,
            .modePath = modePath,
            .write = write,
            .pathExpressionFunction = pathExpressionFunction,
        };

        // If path sync feature is enabled then path feature must be enabled
        CHECK(
            AssertError, !storageFeature(this, storageFeaturePathSync) || storageFeature(this, storageFeaturePath),
            "path feature required");

        // If hardlink feature is enabled then path feature must be enabled
        CHECK(
            AssertError, !storageFeature(this, storageFeatureHardLink) || storageFeature(this, storageFeaturePath),
            "path feature required");

        // If symlink feature is enabled then path feature must be enabled
        CHECK(
            AssertError, !storageFeature(this, storageFeatureSymLink) || storageFeature(this, storageFeaturePath),
            "path feature required");

        // If link features are enabled then linkCreate must be implemented
        CHECK(
            AssertError,
            (!storageFeature(this, storageFeatureSymLink) && !storageFeature(this, storageFeatureHardLink)) ||
            interface.linkCreate != NULL,
            "linkCreate required");
    }
    OBJ_NEW_END();

    FUNCTION_LOG_RETURN(STORAGE, this);
}

/**********************************************************************************************************************************/
FN_EXTERN bool
storageCopy(StorageRead *const source, StorageWrite *const destination)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE_READ, source);
        FUNCTION_LOG_PARAM(STORAGE_WRITE, destination);
    FUNCTION_LOG_END();

    ASSERT(source != NULL);
    ASSERT(destination != NULL);

    bool result = false;

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Open source file
        if (ioReadOpen(storageReadIo(source)))
        {
            // Open the destination file now that we know the source file exists and is readable
            ioWriteOpen(storageWriteIo(destination));

            // Copy data from source to destination
            ioCopyP(storageReadIo(source), storageWriteIo(destination));

            // Close the source and destination files
            ioReadClose(storageReadIo(source));
            ioWriteClose(storageWriteIo(destination));

            // Set result to indicate that the file was copied
            result = true;
        }
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(BOOL, result);
}

/**********************************************************************************************************************************/
FN_EXTERN bool
storageExists(const Storage *const this, const Path *const pathExp, const StorageExistsParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, pathExp);
        FUNCTION_LOG_PARAM(TIMEMSEC, param.timeout);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);

    bool result = false;

    MEM_CONTEXT_TEMP_BEGIN()
    {
        Wait *const wait = waitNew(param.timeout);

        // Loop until file exists or timeout
        do
        {
            // storageInfoLevelBasic is required here because storageInfoLevelExists will not return the type and this function
            // specifically wants to test existence of a *file*, not just the existence of anything with the specified name.
            const StorageInfo info = storageInfoP(
                this, pathExp, .level = storageInfoLevelBasic, .ignoreMissing = true, .followLink = true);

            // Only exists if it is a file
            result = info.exists && info.type == storageTypeFile;
        }
        while (!result && waitMore(wait));
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(BOOL, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Buffer *
storageGet(StorageRead *const file, const StorageGetParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE_READ, file);
        FUNCTION_LOG_PARAM(SIZE, param.exactSize);
    FUNCTION_LOG_END();

    ASSERT(file != NULL);

    Buffer *result = NULL;

    // If the file exists
    if (ioReadOpen(storageReadIo(file)))
    {
        MEM_CONTEXT_TEMP_BEGIN()
        {
            // If exact read
            if (param.exactSize > 0)
            {
                result = bufNew(param.exactSize);
                ioRead(storageReadIo(file), result);

                // If an exact read make sure the size is as expected
                if (bufUsed(result) != param.exactSize)
                    THROW_FMT(FileReadError, "unable to read %zu byte(s) from '%s'", param.exactSize, pathZ(storageReadPath(file)));
            }
            // Else read entire file
            else
            {
                result = bufNew(0);
                Buffer *const read = bufNew(ioBufferSize());

                do
                {
                    // Read data
                    ioRead(storageReadIo(file), read);

                    // Add to result and free read buffer
                    bufCat(result, read);
                    bufUsedZero(read);
                }
                while (!ioReadEof(storageReadIo(file)));
            }

            // Move buffer to parent context on success
            bufMove(result, memContextPrior());
        }
        MEM_CONTEXT_TEMP_END();

        ioReadClose(storageReadIo(file));
    }

    FUNCTION_LOG_RETURN(BUFFER, result);
}

/**********************************************************************************************************************************/
FN_EXTERN StorageInfo
storageInfo(const Storage *const this, const Path *const fileExp, StorageInfoParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, fileExp);
        FUNCTION_LOG_PARAM(ENUM, param.level);
        FUNCTION_LOG_PARAM(BOOL, param.ignoreMissing);
        FUNCTION_LOG_PARAM(BOOL, param.followLink);
        FUNCTION_LOG_PARAM(BOOL, param.noPathEnforce);
    FUNCTION_LOG_END();

    FUNCTION_AUDIT_STRUCT();

    ASSERT(this != NULL);
    ASSERT(this->pub.interface.info != NULL);

    StorageInfo result = {0};

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Build the path
        const Path *const file = storagePathP(this, fileExp, .noEnforce = param.noPathEnforce);

        // Call driver function
        if (param.level == storageInfoLevelDefault)
            param.level = storageFeature(this, storageFeatureInfoDetail) ? storageInfoLevelDetail : storageInfoLevelBasic;

        // If file is / then this is definitely a path so skip the call for drivers that do not support paths and do not provide
        // additional info to return. Also, some object stores (e.g. S3) behave strangely when getting info for /.
        if (pathIsRoot(file) && !storageFeature(this, storageFeaturePath))
        {
            result = (StorageInfo){.level = param.level};
        }
        // Else call the driver
        else
            result = storageInterfaceInfoP(storageDriver(this), file, param.level, .followLink = param.followLink);

        // Error if the file missing and not ignoring
        if (!result.exists && !param.ignoreMissing)
            THROW_FMT(FileOpenError, STORAGE_ERROR_INFO_MISSING, pathZ(file));

        // Dup the strings into the prior context
        MEM_CONTEXT_PRIOR_BEGIN()
        {
            result.linkDestination = strDup(result.linkDestination);
            result.user = strDup(result.user);
            result.group = strDup(result.group);
        }
        MEM_CONTEXT_PRIOR_END();
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(STORAGE_INFO, result);
}

/**********************************************************************************************************************************/
FN_EXTERN StorageIterator *
storageNewItr(const Storage *const this, const Path *const pathExp, StorageNewItrParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, pathExp);
        FUNCTION_LOG_PARAM(ENUM, param.level);
        FUNCTION_LOG_PARAM(BOOL, param.errorOnMissing);
        FUNCTION_LOG_PARAM(BOOL, param.recurse);
        FUNCTION_LOG_PARAM(BOOL, param.nullOnMissing);
        FUNCTION_LOG_PARAM(ENUM, param.sortOrder);
        FUNCTION_LOG_PARAM(STRING, param.expression);
        FUNCTION_LOG_PARAM(BOOL, param.recurse);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(this->pub.interface.list != NULL);
    ASSERT(!param.errorOnMissing || storageFeature(this, storageFeaturePath));

    StorageIterator *result = NULL;

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Info level
        if (param.level == storageInfoLevelDefault)
            param.level = storageFeature(this, storageFeatureInfoDetail) ? storageInfoLevelDetail : storageInfoLevelBasic;

        result = storageItrMove(
            storageItrNew(
                storageDriver(this), storagePathP(this, pathExp), param.level, param.errorOnMissing, param.nullOnMissing,
                param.recurse, param.sortOrder, param.expression),
            memContextPrior());
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(STORAGE_ITERATOR, result);
}

/**********************************************************************************************************************************/
FN_EXTERN void
storageLinkCreate(
    const Storage *const this, const Path *const target, const Path *const linkPath, const StorageLinkCreateParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, target);
        FUNCTION_LOG_PARAM(PATH, linkPath);
        FUNCTION_LOG_PARAM(ENUM, param.linkType);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(this->write);
    ASSERT(target != NULL);
    ASSERT(linkPath != NULL);
    ASSERT(this->pub.interface.linkCreate != NULL);
    ASSERT(
        (param.linkType == storageLinkSym && storageFeature(this, storageFeatureSymLink)) ||
        (param.linkType == storageLinkHard && storageFeature(this, storageFeatureHardLink)));

    MEM_CONTEXT_TEMP_BEGIN()
    {
        storageInterfaceLinkCreateP(storageDriver(this), target, linkPath, .linkType = param.linkType);
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN StringList *
storageList(const Storage *const this, const Path *const pathExp, const StorageListParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, pathExp);
        FUNCTION_LOG_PARAM(BOOL, param.errorOnMissing);
        FUNCTION_LOG_PARAM(BOOL, param.nullOnMissing);
        FUNCTION_LOG_PARAM(STRING, param.expression);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(!param.errorOnMissing || !param.nullOnMissing);

    StringList *result = NULL;

    MEM_CONTEXT_TEMP_BEGIN()
    {
        StorageIterator *const storageItr = storageNewItrP(
            this, pathExp, .level = storageInfoLevelExists, .errorOnMissing = param.errorOnMissing,
            .nullOnMissing = param.nullOnMissing, .expression = param.expression);

        if (storageItr != NULL)
        {
            result = strLstNew();

            while (storageItrMore(storageItr))
                strLstAdd(result, storageItrNext(storageItr).name);

            strLstMove(result, memContextPrior());
        }
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(STRING_LIST, result);
}

/**********************************************************************************************************************************/
FN_EXTERN void
storageMove(const Storage *const this, StorageRead *const source, StorageWrite *const destination)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE_READ, source);
        FUNCTION_LOG_PARAM(STORAGE_WRITE, destination);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(this->pub.interface.move != NULL);
    ASSERT(source != NULL);
    ASSERT(destination != NULL);
    ASSERT(!storageReadIgnoreMissing(source));
    ASSERT(storageType(this) == storageReadType(source));
    ASSERT(storageReadType(source) == storageWriteType(destination));

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // If the file can't be moved it will need to be copied
        if (!storageInterfaceMoveP(storageDriver(this), source, destination))
        {
            // Perform the copy
            storageCopyP(source, destination);

            // Remove the source file
            storageInterfaceRemoveP(storageDriver(this), storageReadPath(source));

            // Sync source path if the destination path was synced. We know the source and destination paths are different because
            // the move did not succeed. This will need updating when drivers other than Posix/CIFS are implemented because there's
            // no way to get coverage on it now.
            if (storageWriteSyncPath(destination))
                storageInterfacePathSyncP(storageDriver(this), pathGetParent(storageReadPath(source)));
        }
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN StorageRead *
storageNewRead(const Storage *const this, const Path *const fileExp, const StorageNewReadParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, fileExp);
        FUNCTION_LOG_PARAM(BOOL, param.ignoreMissing);
        FUNCTION_LOG_PARAM(BOOL, param.compressible);
        FUNCTION_LOG_PARAM(UINT64, param.offset);
        FUNCTION_LOG_PARAM(VARIANT, param.limit);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(param.limit == NULL || varType(param.limit) == varTypeUInt64);

    StorageRead *result;

    MEM_CONTEXT_TEMP_BEGIN()
    {
        result = storageReadMove(
            storageInterfaceNewReadP(
                storageDriver(this), storagePathP(this, fileExp), param.ignoreMissing, .compressible = param.compressible,
                .offset = param.offset, .limit = param.limit),
            memContextPrior());
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(STORAGE_READ, result);
}

/**********************************************************************************************************************************/
FN_EXTERN StorageWrite *
storageNewWrite(const Storage *const this, const Path *const fileExp, const StorageNewWriteParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, fileExp);
        FUNCTION_LOG_PARAM(MODE, param.modeFile);
        FUNCTION_LOG_PARAM(MODE, param.modePath);
        FUNCTION_LOG_PARAM(STRING, param.user);
        FUNCTION_LOG_PARAM(STRING, param.group);
        FUNCTION_LOG_PARAM(INT64, param.timeModified);
        FUNCTION_LOG_PARAM(BOOL, param.noCreatePath);
        FUNCTION_LOG_PARAM(BOOL, param.noSyncFile);
        FUNCTION_LOG_PARAM(BOOL, param.noSyncPath);
        FUNCTION_LOG_PARAM(BOOL, param.noAtomic);
        FUNCTION_LOG_PARAM(BOOL, param.noTruncate);
        FUNCTION_LOG_PARAM(BOOL, param.compressible);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(this->write);
    // noTruncate does not work with atomic writes because a new file is always created for atomic writes
    ASSERT(!param.noTruncate || param.noAtomic);

    StorageWrite *result;

    MEM_CONTEXT_TEMP_BEGIN()
    {
        result = storageWriteMove(
            storageInterfaceNewWriteP(
                storageDriver(this), storagePathP(this, fileExp), .modeFile = param.modeFile != 0 ? param.modeFile : this->modeFile,
                .modePath = param.modePath != 0 ? param.modePath : this->modePath, .user = param.user, .group = param.group,
                .timeModified = param.timeModified, .createPath = !param.noCreatePath, .syncFile = !param.noSyncFile,
                .syncPath = !param.noSyncPath, .atomic = !param.noAtomic, .truncate = !param.noTruncate,
                .compressible = param.compressible),
            memContextPrior());
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(STORAGE_WRITE, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
storagePath(const Storage *const this, const Path *pathExp, const StoragePathParam param)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STORAGE, this);
        FUNCTION_TEST_PARAM(PATH, pathExp);
        FUNCTION_TEST_PARAM(BOOL, param.noEnforce);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    Path *result;

    // If there is no path expression then return the base storage path
    if (pathExp == NULL)
    {
        result = pathDup(this->path);
    }
    else
    {
        // Check if there is a path expression that needs to be evaluated
        if (pathGetRootType(pathExp) == pathRootExpression)
        {
            // Evaluate the path
            Path *pathEvaluated = this->pathExpressionFunction(pathExp);

            // Evaluated path cannot be NULL
            if (pathEvaluated == NULL)
                THROW_FMT(AssertError, "evaluated path '%s' cannot be null", pathZ(pathExp));

            // Evaluated path must be relative
            if (!pathIsRelative(pathEvaluated))
                THROW_FMT(AssertError, "evaluated path '%s' ('%s') must be relative", pathZ(pathExp), pathZ(pathEvaluated));

            result = pathMakeAbsolute(pathEvaluated, this->path);
        }
        // If the path expression is absolute then use it as is
        else if (pathIsAbsolute(pathExp))
        {
            // Make sure the base storage path is contained within the path expression
            if (!param.noEnforce && !pathIsRelativeTo(pathExp, this->path))
                THROW_FMT(AssertError, "absolute path '%s' is not in base path '%s'", pathZ(pathExp), pathZ(this->path));

            result = pathDup(pathExp);
        }
        // Else path expression is relative so combine it with the base storage path
        else
        {
            result = pathMakeAbsolute(pathDup(pathExp), this->path);
        }
    }

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN void
storagePathCreate(const Storage *const this, const Path *const pathExp, const StoragePathCreateParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, pathExp);
        FUNCTION_LOG_PARAM(BOOL, param.errorOnExists);
        FUNCTION_LOG_PARAM(BOOL, param.noParentCreate);
        FUNCTION_LOG_PARAM(MODE, param.mode);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(this->pub.interface.pathCreate != NULL && storageFeature(this, storageFeaturePath));
    ASSERT(this->write);

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Call driver function
        storageInterfacePathCreateP(
            storageDriver(this), storagePathP(this, pathExp), param.errorOnExists, param.noParentCreate,
            param.mode != 0 ? param.mode : this->modePath);
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN bool
storagePathExists(const Storage *const this, const Path *const pathExp)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, pathExp);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(storageFeature(this, storageFeaturePath));

    // storageInfoLevelBasic is required here because storageInfoLevelExists will not return the type and this function specifically
    // wants to test existence of a *path*, not just the existence of anything with the specified name.
    const StorageInfo info = storageInfoP(this, pathExp, .level = storageInfoLevelBasic, .ignoreMissing = true, .followLink = true);

    FUNCTION_LOG_RETURN(BOOL, info.exists && info.type == storageTypePath);
}

/**********************************************************************************************************************************/
FN_EXTERN void
storagePathRemove(const Storage *const this, const Path *const pathExp, const StoragePathRemoveParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, pathExp);
        FUNCTION_LOG_PARAM(BOOL, param.errorOnMissing);
        FUNCTION_LOG_PARAM(BOOL, param.recurse);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(this->write);
    ASSERT(!param.errorOnMissing || storageFeature(this, storageFeaturePath));
    ASSERT(param.recurse || storageFeature(this, storageFeaturePath));

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Build the path
        const Path *const path = storagePathP(this, pathExp);

        // Call driver function
        if (!storageInterfacePathRemoveP(storageDriver(this), path, param.recurse) && param.errorOnMissing)
            THROW_FMT(PathRemoveError, STORAGE_ERROR_PATH_REMOVE_MISSING, pathZ(path));
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN void
storagePathSync(const Storage *const this, const Path *const pathExp)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, pathExp);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(this->write);

    // Not all storage requires path sync so just do nothing if the function is not implemented
    if (this->pub.interface.pathSync != NULL)
    {
        MEM_CONTEXT_TEMP_BEGIN()
        {
            storageInterfacePathSyncP(storageDriver(this), storagePathP(this, pathExp));
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN void
storagePut(StorageWrite *const file, const Buffer *const buffer)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE_WRITE, file);
        FUNCTION_LOG_PARAM(BUFFER, buffer);
    FUNCTION_LOG_END();

    ASSERT(file != NULL);

    ioWriteOpen(storageWriteIo(file));
    ioWrite(storageWriteIo(file), buffer);
    ioWriteClose(storageWriteIo(file));

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN void
storageRemove(const Storage *const this, const Path *const fileExp, const StorageRemoveParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE, this);
        FUNCTION_LOG_PARAM(PATH, fileExp);
        FUNCTION_LOG_PARAM(BOOL, param.errorOnMissing);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(this->write);

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Call driver function
        storageInterfaceRemoveP(storageDriver(this), storagePathP(this, fileExp), .errorOnMissing = param.errorOnMissing);
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN void
storageToLog(const Storage *const this, StringStatic *const debugLog)
{
    strStcCat(debugLog, "{type: "),
    strStcResultSizeInc(debugLog, strIdToLog(storageType(this), strStcRemains(debugLog), strStcRemainsSize(debugLog)));
    strStcCat(debugLog, ", path: ");
    strStcResultSizeInc(debugLog, FUNCTION_LOG_PATH_FORMAT(this->path, strStcRemains(debugLog), strStcRemainsSize(debugLog)));
    strStcFmt(debugLog, ", write: %s}", cvtBoolToConstZ(this->write));
}
