#include "build.auto.h"

#include "common/type/list.h"
#include "storage/vfs/mountPointList.h"

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
struct StorageVfsMountPointList
{
    List *list;
};

/**********************************************************************************************************************************/
static int
storageVfsMountPointListComparator(const void *item1, const void *item2)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM_P(VOID, item1);
        FUNCTION_TEST_PARAM_P(VOID, item2);
    FUNCTION_TEST_END();

    ASSERT(item1 != NULL);
    ASSERT(item2 != NULL);

    FUNCTION_TEST_RETURN(INT, lstComparatorStr(((StorageVfsMountPoint *) item1)->expression, ((StorageVfsMountPoint *) item2)->expression));
}

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN StorageVfsMountPointList *
storageVfsMountPointListNew(void)
{
    FUNCTION_TEST_BEGIN();
    FUNCTION_TEST_END();

    OBJ_NEW_BEGIN(StorageVfsMountPointList, .childQty = MEM_CONTEXT_QTY_MAX)
    {
        *this = (StorageVfsMountPointList)
        {
            .list = lstNewP(sizeof(StorageVfsMountPoint), .comparator = storageVfsMountPointListComparator),
        };
    }
    OBJ_NEW_END();

    FUNCTION_TEST_RETURN(STORAGE_VFS_MOUNT_POINT_LIST, this);
}

/***********************************************************************************************************************************
Getters/Setters
***********************************************************************************************************************************/
FN_EXTERN bool
storageVfsMountPointListEmpty(const StorageVfsMountPointList *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STORAGE_VFS_MOUNT_POINT_LIST, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(BOOL, lstEmpty(this->list));
}

/**********************************************************************************************************************************/
FN_EXTERN unsigned int
storageVfsMountPointListSize(const StorageVfsMountPointList *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STORAGE_VFS_MOUNT_POINT_LIST, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(UINT, lstSize(this->list));
}

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
FN_EXTERN void
storageVfsMountPointListClear(StorageVfsMountPointList *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STORAGE_VFS_MOUNT_POINT_LIST, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    lstClear(this->list);

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN void
storageVfsMountPointListInsert(StorageVfsMountPointList *const this, unsigned int idx, const StorageVfsMountPoint *item)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STORAGE_VFS_MOUNT_POINT_LIST, this);
        FUNCTION_TEST_PARAM(UINT, idx);
        FUNCTION_TEST_PARAM(STORAGE_VFS_MOUNT_POINT, item);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(item != NULL);

    MEM_CONTEXT_OBJ_BEGIN(this->list);
    {
        StorageVfsMountPoint mountPoint = {
            .expression = strDup(item->expression),
            .storage = objMove(item->storage, memContextCurrent()),
            .callback = item->callback,
        };

        lstInsert(this->list, idx, &mountPoint);
    }
    MEM_CONTEXT_OBJ_END();

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
FN_EXTERN StorageVfsMountPoint *
storageVfsMountPointListGet(StorageVfsMountPointList *const this, unsigned int idx)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STORAGE_VFS_MOUNT_POINT_LIST, this);
        FUNCTION_TEST_PARAM(UINT, idx);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(STORAGE_VFS_MOUNT_POINT, lstGet(this->list, idx));
}

/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
FN_EXTERN void
storageVfsMountPointListToLog(const StorageVfsMountPointList *const this, StringStatic *const debugLog)
{
    strStcFmt(debugLog, "{size: %u}", lstSize(this->list));
}

