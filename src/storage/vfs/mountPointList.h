/***********************************************************************************************************************************
VFS Storage mount point list
***********************************************************************************************************************************/
#ifndef STORAGE_VFS_MOUNTPOINTLIST_H
#define STORAGE_VFS_MOUNTPOINTLIST_H

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
typedef struct StorageVfsMountPointList StorageVfsMountPointList;

#include "storage/vfs/mountPoint.h"

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN StorageVfsMountPointList *storageVfsMountPointListNew(void);

/***********************************************************************************************************************************
Getters/Setters
***********************************************************************************************************************************/
//typedef struct StorageVfsMountPointListPub
//{
//    List *list;
//} StorageVfsMountPointListPub;

FN_EXTERN bool storageVfsMountPointListEmpty(const StorageVfsMountPointList *this);
FN_EXTERN unsigned int storageVfsMountPointListSize(const StorageVfsMountPointList *this);

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
FN_EXTERN void storageVfsMountPointListClear(StorageVfsMountPointList *this);
FN_EXTERN void storageVfsMountPointListInsert(StorageVfsMountPointList *this, unsigned int idx, const StorageVfsMountPoint *item);

FN_INLINE_ALWAYS void
storageVfsMountPointListAdd(StorageVfsMountPointList *this, const StorageVfsMountPoint *item)
{
    storageVfsMountPointListInsert(this, storageVfsMountPointListSize(this), item);
}

FN_EXTERN StorageVfsMountPoint *storageVfsMountPointListGet(StorageVfsMountPointList *this, unsigned int idx);

// Move to a new parent mem context
FN_INLINE_ALWAYS StorageVfsMountPointList *
storageVfsMountPointListMove(StorageVfsMountPointList *const this, MemContext *const parentNew)
{
    return objMove(this, parentNew);
}

/***********************************************************************************************************************************
Destructor
***********************************************************************************************************************************/
FN_INLINE_ALWAYS void
storageVfsMountPointListFree(StorageVfsMountPointList *const this)
{
    objFree(this);
}

/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
FN_EXTERN void storageVfsMountPointListToLog(const StorageVfsMountPointList *this, StringStatic *debugLog);

#define FUNCTION_LOG_STORAGE_VFS_MOUNT_POINT_LIST_TYPE                                                                                             \
    StorageVfsMountPointList *
#define FUNCTION_LOG_STORAGE_VFS_MOUNT_POINT_LIST_FORMAT(value, buffer, bufferSize)                                                                \
    FUNCTION_LOG_OBJECT_FORMAT(value, storageVfsMountPointListToLog, buffer, bufferSize)

#endif
