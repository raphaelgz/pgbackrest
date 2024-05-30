#include "build.auto.h"

#include "common/log.h"
#include "storage/vfs/mountPoint.h"

///***********************************************************************************************************************************
//Object type
//***********************************************************************************************************************************/
//struct StorageVfsMountPoint
//{
//    String *expression;
//    Storage *storage;
//    StorageVfsResolvePathExpressionCallback *callback;
//};
//
//FN_EXTERN StorageVfsMountPoint *
//storageVfsMountPointNew(const String *const expression, Storage *const storage, StorageVfsResolvePathExpressionCallback *const callback)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(STRING, expression);
//        FUNCTION_TEST_PARAM(STORAGE, storage);
//        FUNCTION_TEST_PARAM(FUNCTIONP, callback);
//    FUNCTION_TEST_END();
//
//    ASSERT(expression != NULL && !strEmpty(expression));
//    ASSERT(storage != NULL);
//    ASSERT(callback != NULL);
//
//    OBJ_NEW_BEGIN(StorageVfsMountPoint, .childQty = MEM_CONTEXT_QTY_MAX);
//    {
//        *this = (StorageVfsMountPoint)
//        {
//            .expression = strDup(expression),
//            .storage = objMove(storage, memContextCurrent()),
//            .callback = callback,
//        };
//    }
//    OBJ_NEW_END();
//
//    FUNCTION_TEST_RETURN(VFS_MOUNT_POINT, this);
//}
