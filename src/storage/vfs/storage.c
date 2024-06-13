#include "build.auto.h"

#include "common/log.h"
#include "common/path.h"
#include "config/config.h"
#include "protocol/helper.h"
#include "storage/vfs/storage.intern.h"

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
typedef struct
{
    const String *expression;
    const Path *virtualBase;
    const Storage *storage;
    StorageVfsResolvePathExpressionCallback *callback;
} StorageVfsInternalMountPoint;

struct StorageVfs
{
    STORAGE_COMMON_MEMBER;
    StorageVfsInternalMountPoint *mountPoints;
    unsigned int mountPointsSize;
};

/**********************************************************************************************************************************/
static const StorageVfsInternalMountPoint *
storageVfsFindMountPointByExpression(const StorageVfs *const this, const String *const expression)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STORAGE_VFS, this);
        FUNCTION_TEST_PARAM(STRING, expression);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(expression != NULL);

    const StorageVfsInternalMountPoint *result = NULL;

    for (unsigned int idx = 0; idx < this->mountPointsSize; idx++)
    {
        if (strEq(this->mountPoints[idx].expression, expression))
        {
            result = &this->mountPoints[idx];
            break;
        }
    }

    FUNCTION_TEST_RETURN_TYPE(StorageVfsInternalMountPoint *, result);
}

/**********************************************************************************************************************************/
static const StorageVfsInternalMountPoint *
storageVfsFindMountPointByPath(const StorageVfs *const this, const Path *const path)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STORAGE_VFS, this);
        FUNCTION_TEST_PARAM(PATH, path);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(path != NULL);

    const StorageVfsInternalMountPoint *result = NULL;

    for (unsigned int idx = 0; idx < this->mountPointsSize; idx++)
    {
        if (pathIsRelativeTo(path, this->mountPoints[idx].virtualBase))
        {
            result = &this->mountPoints[idx];
            break;
        }
    }

    FUNCTION_TEST_RETURN_TYPE(StorageVfsInternalMountPoint *, result);
}

/**********************************************************************************************************************************/
static StorageInfo
storageVfsInfo(THIS_VOID, const Path *const file, StorageInfoLevel level, StorageInterfaceInfoParam param)
{
    THIS(StorageVfs);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_VFS, this);
        FUNCTION_LOG_PARAM(PATH, file);
        FUNCTION_LOG_PARAM(ENUM, level);
        FUNCTION_LOG_PARAM(BOOL, param.followLink);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);

    StorageInfo result = {.level = level};
    const StorageVfsInternalMountPoint *mountPoint = storageVfsFindMountPointByPath(this, file);

    if (mountPoint != NULL)
    {
        MEM_CONTEXT_TEMP_BEGIN();
        {
            const Path *relativePath = pathMakeRelativeTo(pathDup(file), mountPoint->virtualBase);
            const Path *realPath = storagePathP(mountPoint->storage, relativePath);

            MEM_CONTEXT_PRIOR_BEGIN();
            {
                result = storageInterfaceInfoP(storageDriver(mountPoint->storage), realPath, level, .followLink = param.followLink);

                /* FIXME: should we rebase the path? */
//                result.name
            }
            MEM_CONTEXT_PRIOR_END();
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_LOG_RETURN(STORAGE_INFO, result);
}

/**********************************************************************************************************************************/
static StorageList *
storageVfsList(THIS_VOID, const Path *const path, StorageInfoLevel level, StorageInterfaceListParam param)
{
    THIS(StorageVfs);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_VFS, this);
        FUNCTION_LOG_PARAM(PATH, path);
        FUNCTION_LOG_PARAM(ENUM, level);
        FUNCTION_LOG_PARAM(STRING, param.expression);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(path != NULL);

    StorageList *result = NULL;
    const StorageVfsInternalMountPoint *mountPoint = storageVfsFindMountPointByPath(this, path);

    if (mountPoint != NULL)
    {
        MEM_CONTEXT_TEMP_BEGIN();
        {
            const Path *relativePath = pathMakeRelativeTo(pathDup(path), mountPoint->virtualBase);
            const Path *realPath = storagePathP(mountPoint->storage, relativePath);

            MEM_CONTEXT_PRIOR_BEGIN();
            {
                result = storageInterfaceListP(storageDriver(mountPoint->storage), realPath, level, .expression = param.expression);

                /* FIXME: should we rebase all the paths? */
//                storageLstGet(result, 0).name
            }
            MEM_CONTEXT_PRIOR_END();
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_LOG_RETURN(STORAGE_LIST, result);
}

/**********************************************************************************************************************************/
//static StorageRead *
//storageVfsNewRead(THIS_VOID, const String *const file, bool ignoreMissing, StorageInterfaceNewReadParam param)
//{
//    THIS(StorageVfs);
//
//    FUNCTION_LOG_BEGIN(logLevelTrace);
//        FUNCTION_LOG_PARAM(STORAGE_VFS, this);
//        FUNCTION_LOG_PARAM(STRING, file);
//        FUNCTION_LOG_PARAM(BOOL, ignoreMissing);
//        FUNCTION_LOG_PARAM(BOOL, param.compressible);
//        FUNCTION_LOG_PARAM(VARIANT, param.limit);
//        FUNCTION_LOG_PARAM(UINT64, param.offset);
//    FUNCTION_LOG_END();
//
//    ASSERT(this != NULL);
//    ASSERT(file != NULL);
//
//    StorageRead *result = NULL;
//    const StorageVfsInternalMountPoint *mountPoint = storageVfsFindMountPointByPath(this, file);
//
//    if (mountPoint != NULL)
//    {
//        MEM_CONTEXT_TEMP_BEGIN();
//        {
//            String *relativePath = pathMakeRelativeTo(mountPoint->virtualBase, file);
//            String *realPath = storagePathP(mountPoint->storage, relativePath);
//
//            MEM_CONTEXT_PRIOR_BEGIN();
//            {
//                result = storageInterfaceNewReadP(
//                    storageDriver(mountPoint->storage), realPath, ignoreMissing, .offset = param.offset,
//                    .limit = param.limit, .compressible = param.compressible);
//            }
//            MEM_CONTEXT_PRIOR_END();
//        }
//        MEM_CONTEXT_TEMP_END();
//    }
//
//    FUNCTION_LOG_RETURN(STORAGE_READ, result);
//}

/**********************************************************************************************************************************/
//static StorageWrite *
//storageVfsNewWrite(void *thisVoid, const String *const file, StorageInterfaceNewWriteParam param)
//{
//    THIS(StorageVfs);
//
//    FUNCTION_LOG_BEGIN(logLevelTrace);
//        FUNCTION_LOG_PARAM(STORAGE_VFS, this);
//        FUNCTION_LOG_PARAM(STRING, file);
//    FUNCTION_LOG_END();
//
//    ASSERT(this != NULL);
//    ASSERT(file != NULL);
//
//    StorageWrite *result = NULL;
//    const StorageVfsInternalMountPoint *mountPoint = storageVfsFindMountPointByPath(this, file);
//
//    if (mountPoint != NULL)
//    {
//        MEM_CONTEXT_TEMP_BEGIN();
//        {
//            String *relativePath = pathMakeRelativeTo(mountPoint->virtualBase, file);
//            String *realPath = storagePathP(mountPoint->storage, relativePath);
//
//            MEM_CONTEXT_PRIOR_BEGIN();
//            {
//                result = storageInterfaceNewWriteP(
//                    storageDriver(mountPoint->storage), realPath, .compressible = param.compressible,
//                    .modePath = param.modePath, .modeFile = param.modeFile, .atomic = param.atomic,
//                    .createPath = param.createPath, .group = param.group, .syncFile = param.syncFile, .syncPath = param.syncFile,
//                    .timeModified = param.timeModified, .truncate = param.truncate, .user = param.user);
//            }
//            MEM_CONTEXT_PRIOR_END();
//        }
//        MEM_CONTEXT_TEMP_END();
//    }
//
//    FUNCTION_LOG_RETURN(STORAGE_WRITE, result);
//}

/**********************************************************************************************************************************/
static bool storageVfsPathRemove(THIS_VOID, const Path *const path, bool recurse, StorageInterfacePathRemoveParam param)
{
    THIS(StorageVfs);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_VFS, this);
        FUNCTION_LOG_PARAM(PATH, path);
        (void) param;
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(path != NULL);

    bool result = true;
    const StorageVfsInternalMountPoint *mountPoint = storageVfsFindMountPointByPath(this, path);

    if (mountPoint != NULL)
    {
        MEM_CONTEXT_TEMP_BEGIN();
        {
            const Path *relativePath = pathMakeRelativeTo(pathDup(path), mountPoint->virtualBase);
            const Path *realPath = storagePathP(mountPoint->storage, relativePath);

            result = storageInterfacePathRemoveP(storageDriver(mountPoint->storage), realPath, recurse);
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_LOG_RETURN(BOOL, result);
}

/**********************************************************************************************************************************/
static void
storageVfsRemove(THIS_VOID, const Path *const file, StorageInterfaceRemoveParam param)
{
    THIS(StorageVfs);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_VFS, this);
        FUNCTION_LOG_PARAM(PATH, file);
        FUNCTION_LOG_PARAM(BOOL, param.errorOnMissing);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(file != NULL);

    const StorageVfsInternalMountPoint *mountPoint = storageVfsFindMountPointByPath(this, file);

    if (mountPoint != NULL)
    {
        MEM_CONTEXT_TEMP_BEGIN();
        {
            const Path *relativePath = pathMakeRelativeTo(pathDup(file), mountPoint->virtualBase);
            const Path *realPath = storagePathP(mountPoint->storage, relativePath);

            storageInterfaceRemoveP(storageDriver(mountPoint->storage), realPath, .errorOnMissing = param.errorOnMissing);
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
static void
storageVfsLinkCreate(THIS_VOID, const Path *const target, const Path *const linkPath, StorageInterfaceLinkCreateParam param)
{
    THIS(StorageVfs);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_VFS, this);
        FUNCTION_LOG_PARAM(PATH, target);
        FUNCTION_LOG_PARAM(PATH, linkPath);
        FUNCTION_LOG_PARAM(ENUM, param.linkType);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(target != NULL);
    ASSERT(linkPath != NULL);

    const StorageVfsInternalMountPoint *mountPoint = storageVfsFindMountPointByPath(this, linkPath);

    if (mountPoint != NULL)
    {
        MEM_CONTEXT_TEMP_BEGIN();
        {
            const Path *relativePath = pathMakeRelativeTo(pathDup(linkPath), mountPoint->virtualBase);
            const Path *realPath = storagePathP(mountPoint->storage, relativePath);

            storageInterfaceLinkCreateP(storageDriver(mountPoint->storage), target, realPath, .linkType = param.linkType);
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
//static bool
//storageVfsMove(THIS_VOID, StorageRead *const source, StorageWrite *const destination, StorageInterfaceMoveParam param)
//{
//    THIS(StorageVfs);
//
//    FUNCTION_LOG_BEGIN(logLevelTrace);
//        FUNCTION_LOG_PARAM(STORAGE_VFS, this);
//        FUNCTION_LOG_PARAM(STORAGE_READ, source);
//        FUNCTION_LOG_PARAM(STORAGE_WRITE, destination);
//        (void)param;
//    FUNCTION_LOG_END();
//
//    ASSERT(this != NULL);
//    ASSERT(source != NULL);
//    ASSERT(destination != NULL);
//
//    bool result = true;
//    const StorageVfsInternalMountPoint *mountPoint = storageVfsFindMountPointByPath(this, linkPath);
//
//    if (mountPoint != NULL)
//    {
//        MEM_CONTEXT_TEMP_BEGIN();
//        {
//            String *relativePath = pathMakeRelativeTo(mountPoint->virtualBase, linkPath);
//            String *realPath = storagePathP(mountPoint->storage, relativePath);
//
//            storageInterfaceLinkCreateP(storageDriver(mountPoint->storage), target, linkPath, .linkType = param.linkType);
//        }
//        MEM_CONTEXT_TEMP_END();
//    }
//}

/**********************************************************************************************************************************/
static void
storageVfsPathCreate(
    THIS_VOID, const Path *const path, bool errorOnExists, bool noParentCreate, mode_t mode,
    StorageInterfacePathCreateParam param)
{
    THIS(StorageVfs);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_VFS, this);
        FUNCTION_LOG_PARAM(PATH, path);
        FUNCTION_LOG_PARAM(BOOL, errorOnExists);
        FUNCTION_LOG_PARAM(BOOL, noParentCreate);
        FUNCTION_LOG_PARAM(MODE, mode);
        (void) param;
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(path != NULL);

    const StorageVfsInternalMountPoint *mountPoint = storageVfsFindMountPointByPath(this, path);

    if (mountPoint != NULL)
    {
        MEM_CONTEXT_TEMP_BEGIN();
        {
            const Path *relativePath = pathMakeRelativeTo(pathDup(path), mountPoint->virtualBase);
            const Path *realPath = storagePathP(mountPoint->storage, relativePath);

            storageInterfacePathCreateP(storageDriver(mountPoint->storage), realPath, errorOnExists, noParentCreate, mode);
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
static void
storageVfsPathSync(THIS_VOID, const Path *const path, StorageInterfacePathSyncParam param)
{
    THIS(StorageVfs);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_VFS, this);
        FUNCTION_LOG_PARAM(PATH, path);
        (void) param;
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(path != NULL);

    const StorageVfsInternalMountPoint *mountPoint = storageVfsFindMountPointByPath(this, path);

    /* FIXME: what to do if the mount point is not found? */
    if (mountPoint != NULL)
    {
        MEM_CONTEXT_TEMP_BEGIN();
        {
            const Path *relativePath = pathMakeRelativeTo(pathDup(path), mountPoint->virtualBase);
            const Path *realPath = storagePathP(mountPoint->storage, relativePath);

            storageInterfacePathSyncP(storageDriver(mountPoint->storage), realPath);
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
static Path *
storageVfsResolvePathExpression(THIS_VOID, const Path *const pathExp, StorageInterfaceResolvePathExpressionParam param)
{
    THIS(StorageVfs);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_VFS, this);
        FUNCTION_LOG_PARAM(PATH, pathExp);
        (void) param;
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(pathExp != NULL);

    Path *result = NULL;
    const StorageVfsInternalMountPoint *mountPoint = storageVfsFindMountPointByExpression(this, pathGetRoot(pathExp));

    if (mountPoint != NULL)
    {
        Path *resolvedPath;

        // If there is no special callback associated with the mount point just remove the root expression
        if (mountPoint->callback == NULL)
            resolvedPath = pathResolveExpressionZ(pathExp, ".");
        else
            resolvedPath = mountPoint->callback(pathExp);

        if (resolvedPath != NULL)
        {
            if (!pathIsRelative(resolvedPath))
                THROW_FMT(AssertError, "the path expression resolver callback for '%s' must return a relative path", pathZ(pathExp));

            result = pathMakeAbsolute(resolvedPath, mountPoint->virtualBase);
        }
    }
    else
        THROW_FMT(AssertError, "invalid expression '%s'", pathZ(pathExp));

    FUNCTION_LOG_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
static const StorageInterface storageInterfaceVfs =
{
    .feature = 1 << storageFeaturePathExpressionResolver,

    .info = storageVfsInfo,
    .list = storageVfsList,
//    .newRead = storageVfsNewRead,
//    .newWrite = storageVfsNewWrite,
    .pathRemove = storageVfsPathRemove,
    .remove = storageVfsRemove,
    .linkCreate = storageVfsLinkCreate,
//    .move = storageVfsMove,
    .pathCreate = storageVfsPathCreate,
    .pathSync = storageVfsPathSync,
    .resolvePathExpression = storageVfsResolvePathExpression,
};

FN_EXTERN Storage *
storageVfsNew(StorageVfsMountPointList *mountPoints, StorageVfsNewParam param)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STORAGE_VFS_MOUNT_POINT_LIST, mountPoints);
        (void) param;
    FUNCTION_LOG_END();

    ASSERT(mountPoints != NULL && !storageVfsMountPointListEmpty(mountPoints));

    OBJ_NEW_BEGIN(StorageVfs, .allocQty = 1, .childQty = MEM_CONTEXT_QTY_MAX);
    {
        *this = (StorageVfs)
        {
            .interface = storageInterfaceVfs,
            .mountPoints = memNew(sizeof(StorageVfsInternalMountPoint) * storageVfsMountPointListSize(mountPoints)),
            .mountPointsSize = storageVfsMountPointListSize(mountPoints),
        };

        for (unsigned int idx = 0; idx < storageVfsMountPointListSize(mountPoints); idx++)
        {
            char storageTypeZ[STRID_MAX + 1];
            StorageVfsMountPoint *mountPoint = storageVfsMountPointListGet(mountPoints, idx);

            strIdToZ(storageType(mountPoint->storage), storageTypeZ);

            if (storageNeedsPathExpression(mountPoint->storage))
                THROW_FMT(AssertError, "the storage '%s' cannot be used as a mount point", storageTypeZ);

            this->interface.feature &= storageInterface(mountPoint->storage).feature;

            this->mountPoints[idx].expression  = strDup(mountPoint->expression);
            this->mountPoints[idx].storage     = objMove(mountPoint->storage, memContextCurrent());
            this->mountPoints[idx].callback    = mountPoint->callback;
            this->mountPoints[idx].virtualBase = pathNewFmt("/VFS/mount-point-%s", strZ(mountPoint->virtualFolder));
        }

        storageVfsMountPointListClear(mountPoints);
    }
    OBJ_NEW_END();

    FUNCTION_LOG_RETURN(
        STORAGE,
        storageNew(STORAGE_VFS_TYPE, NULL, STORAGE_MODE_FILE_DEFAULT, STORAGE_MODE_PATH_DEFAULT,
                   true /* FIXME: create storageIsWritable */, this, this->interface));
}
