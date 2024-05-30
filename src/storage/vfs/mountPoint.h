/***********************************************************************************************************************************
VFS Storage mount point
***********************************************************************************************************************************/
#ifndef STORAGE_VFS_MOUNTPOINT_H
#define STORAGE_VFS_MOUNTPOINT_H

#include "common/path.h"
#include "common/type/string.h"
#include "storage/storage.h"

/***********************************************************************************************************************************
Resolver callback
***********************************************************************************************************************************/
typedef Path *StorageVfsResolvePathExpressionCallback(const Path *pathExp);

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
typedef struct StorageVfsMountPoint
{
    Storage *storage;
    const String *expression;
    const String *virtualFolder;
    StorageVfsResolvePathExpressionCallback *callback;
} StorageVfsMountPoint;

//typedef struct StorageVfsMountPoint StorageVfsMountPoint;
//
///***********************************************************************************************************************************
//Constructors
//***********************************************************************************************************************************/
//FN_EXTERN StorageVfsMountPoint *storageVfsMountPointNew(const String *expression, Storage *storage, StorageVfsResolvePathExpressionCallback *callback);
//
///***********************************************************************************************************************************
//Destructor
//***********************************************************************************************************************************/
//FN_INLINE_ALWAYS void
//storageVfsMountPointFree(StorageVfsMountPoint *const this)
//{
//    objFree(this);
//}
//
/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
#define FUNCTION_LOG_STORAGE_VFS_MOUNT_POINT_TYPE                                                                                          \
    StorageVfsMountPoint *
#define FUNCTION_LOG_STORAGE_VFS_MOUNT_POINT_FORMAT(value, buffer, bufferSize)                                                             \
    objNameToLog(value, "StorageVfsMountPoint", buffer, bufferSize)

#endif
