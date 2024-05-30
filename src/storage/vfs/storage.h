/***********************************************************************************************************************************
"Virtual File System" Storage
***********************************************************************************************************************************///
#ifndef STORAGE_VFS_STORAGE_H
#define STORAGE_VFS_STORAGE_H

#include "storage/storage.h"
#include "storage/vfs/mountPointList.h"

/***********************************************************************************************************************************
Storage type
***********************************************************************************************************************************/
#define STORAGE_VFS_TYPE                                           STRID5("vfs", 0x4cd60)

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
typedef struct StorageVfsNewParam
{
    VAR_PARAM_HEADER;
} StorageVfsNewParam;

#define storageVfsNewP(mountPoints, ...)                                                                                                \
    storageVfsNew(mountPoints, (StorageVfsNewParam){VAR_PARAM_INIT, __VA_ARGS__})

FN_EXTERN Storage *storageVfsNew(StorageVfsMountPointList *mountPoints, StorageVfsNewParam param);

#endif
