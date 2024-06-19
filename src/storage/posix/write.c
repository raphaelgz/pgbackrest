/***********************************************************************************************************************************
Posix Storage File write
***********************************************************************************************************************************/
#include "build.auto.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <utime.h>

#include "common/debug.h"
#include "common/io/write.h"
#include "common/log.h"
#include "common/type/object.h"
#include "common/user.h"
#include "storage/posix/write.h"
#include "storage/write.intern.h"

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
typedef struct StorageWritePosix
{
    StorageWriteInterface interface;                                // Interface
    StoragePosix *storage;                                          // Storage that created this object

    const Path *fileTmp;
    const Path *directory;
    int fd;                                                         // File descriptor
} StorageWritePosix;

/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
#define FUNCTION_LOG_STORAGE_WRITE_POSIX_TYPE                                                                                      \
    StorageWritePosix *
#define FUNCTION_LOG_STORAGE_WRITE_POSIX_FORMAT(value, buffer, bufferSize)                                                         \
    objNameToLog(value, "StorageWritePosix", buffer, bufferSize)

/***********************************************************************************************************************************
File open constants

Since open is called more than once use constants to make sure these parameters are always the same
***********************************************************************************************************************************/
#define FILE_OPEN_PURPOSE                                           "write"

/***********************************************************************************************************************************
Close file descriptor
***********************************************************************************************************************************/
static void
storageWritePosixFreeResource(THIS_VOID)
{
    THIS(StorageWritePosix);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_WRITE_POSIX, this);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);

    THROW_ON_SYS_ERROR_FMT(close(this->fd) == -1, FileCloseError, STORAGE_ERROR_WRITE_CLOSE, pathZ(this->fileTmp));

    FUNCTION_LOG_RETURN_VOID();
}

/***********************************************************************************************************************************
Open the file
***********************************************************************************************************************************/
static const char *
storageWritePosixOpenOwner(const String *const owner, const char *const defaultOwner)
{
    return owner == NULL ? defaultOwner : strZ(owner);
}

static const char *
storageWritePosixOpenOwnerId(const String *const owner, const unsigned int ownerId)
{
    return owner == NULL ? "" : zNewFmt("[%u]", ownerId);
}

static void
storageWritePosixOpen(THIS_VOID)
{
    THIS(StorageWritePosix);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_WRITE_POSIX, this);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(this->fd == -1);

    // Open the file
    const int flags = O_CREAT | O_WRONLY | (this->interface.truncate ? O_TRUNC : 0);

    this->fd = open(pathZ(this->fileTmp), flags, this->interface.modeFile);

    // Attempt to create the path if it is missing
    if (this->fd == -1 && errno == ENOENT && this->interface.createPath)                                            // {vm_covered}
    {
        // Create the path
        storageInterfacePathCreateP(this->storage, this->directory, false, false, this->interface.modePath);

        // Open file again
        this->fd = open(pathZ(this->fileTmp), flags, this->interface.modeFile);
    }

    // Handle errors
    if (this->fd == -1)
    {
        if (errno == ENOENT)                                                                                        // {vm_covered}
            THROW_FMT(FileMissingError, STORAGE_ERROR_WRITE_MISSING, pathZ(this->interface.path));
        else
            THROW_SYS_ERROR_FMT(FileOpenError, STORAGE_ERROR_WRITE_OPEN, pathZ(this->interface.path));               // {vm_covered}
    }

    // Set free callback to ensure the file descriptor is freed
    memContextCallbackSet(objMemContext(this), storageWritePosixFreeResource, this);

    // Update user/group owner
    if (this->interface.user != NULL || this->interface.group != NULL)
    {
        MEM_CONTEXT_TEMP_BEGIN()
        {
            const StorageInfo info = storageInterfaceInfoP(
                this->storage, this->fileTmp, storageInfoLevelDetail, .followLink = true);
            ASSERT(info.exists);
            uid_t updateUserId = userIdFromName(this->interface.user);
            gid_t updateGroupId = groupIdFromName(this->interface.group);

            if (updateUserId == (uid_t)-1)
                updateUserId = info.userId;

            if (updateGroupId == (gid_t)-1)
                updateGroupId = info.groupId;

            // Continue if one of the owners would be changed
            if (updateUserId != info.userId || updateGroupId != info.groupId)
            {
                THROW_ON_SYS_ERROR_FMT(
                    chown(pathZ(this->fileTmp), updateUserId, updateGroupId) == -1, FileOwnerError,
                    "unable to set ownership for '%s' to %s%s:%s%s from %s[%u]:%s[%u]", pathZ(this->fileTmp),
                    storageWritePosixOpenOwner(this->interface.user, "[none]"),
                    storageWritePosixOpenOwnerId(this->interface.user, updateUserId),
                    storageWritePosixOpenOwner(this->interface.group, "[none]"),
                    storageWritePosixOpenOwnerId(this->interface.group, updateGroupId),
                    storageWritePosixOpenOwner(info.user, "[unknown]"), info.userId,
                    storageWritePosixOpenOwner(info.group, "[unknown]"), info.groupId);
            }
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_LOG_RETURN_VOID();
}

/***********************************************************************************************************************************
Write to the file
***********************************************************************************************************************************/
static void
storageWritePosix(THIS_VOID, const Buffer *const buffer)
{
    THIS(StorageWritePosix);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_WRITE_POSIX, this);
        FUNCTION_LOG_PARAM(BUFFER, buffer);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(buffer != NULL);
    ASSERT(this->fd != -1);

    // Write the data
    if (write(this->fd, bufPtrConst(buffer), bufUsed(buffer)) != (ssize_t)bufUsed(buffer))
        THROW_SYS_ERROR_FMT(FileWriteError, "unable to write '%s'", pathZ(this->fileTmp));

    FUNCTION_LOG_RETURN_VOID();
}

/***********************************************************************************************************************************
Close the file
***********************************************************************************************************************************/
static void
storageWritePosixClose(THIS_VOID)
{
    THIS(StorageWritePosix);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_WRITE_POSIX, this);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);

    // Close if the file has not already been closed
    if (this->fd != -1)
    {
        // Sync the file
        if (this->interface.syncFile)
            THROW_ON_SYS_ERROR_FMT(fsync(this->fd) == -1, FileSyncError, STORAGE_ERROR_WRITE_SYNC, pathZ(this->fileTmp));

        // Close the file
        memContextCallbackClear(objMemContext(this));
        THROW_ON_SYS_ERROR_FMT(close(this->fd) == -1, FileCloseError, STORAGE_ERROR_WRITE_CLOSE, pathZ(this->fileTmp));
        this->fd = -1;

        // Update modified time
        if (this->interface.timeModified != 0)
        {
            THROW_ON_SYS_ERROR_FMT(
                utime(
                    pathZ(this->fileTmp),
                    &((struct utimbuf){.actime = this->interface.timeModified, .modtime = this->interface.timeModified})) == -1,
                FileInfoError, "unable to set time for '%s'", pathZ(this->fileTmp));
        }

        // Rename from temp file
        if (this->interface.atomic)
        {
            if (rename(pathZ(this->fileTmp), pathZ(this->interface.path)) == -1)
                THROW_SYS_ERROR_FMT(FileMoveError, "unable to move '%s' to '%s'", pathZ(this->fileTmp), pathZ(this->interface.path));
        }

        // Sync the path
        if (this->interface.syncPath)
            storageInterfacePathSyncP(this->storage, this->directory);
    }

    FUNCTION_LOG_RETURN_VOID();
}

/***********************************************************************************************************************************
Get file descriptor
***********************************************************************************************************************************/
static int
storageWritePosixFd(const THIS_VOID)
{
    THIS(const StorageWritePosix);

    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STORAGE_WRITE_POSIX, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(INT, this->fd);
}

/**********************************************************************************************************************************/
FN_EXTERN StorageWrite *
storageWritePosixNew(
    StoragePosix *const storage, const Path *const file, const mode_t modeFile, const mode_t modePath, const String *const user,
    const String *const group, const time_t timeModified, const bool createPath, const bool syncFile, const bool syncPath,
    const bool atomic, const bool truncate)
{
    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(STORAGE_POSIX, storage);
        FUNCTION_LOG_PARAM(PATH, file);
        FUNCTION_LOG_PARAM(MODE, modeFile);
        FUNCTION_LOG_PARAM(MODE, modePath);
        FUNCTION_LOG_PARAM(STRING, user);
        FUNCTION_LOG_PARAM(STRING, group);
        FUNCTION_LOG_PARAM(TIME, timeModified);
        FUNCTION_LOG_PARAM(BOOL, createPath);
        FUNCTION_LOG_PARAM(BOOL, syncFile);
        FUNCTION_LOG_PARAM(BOOL, syncPath);
        FUNCTION_LOG_PARAM(BOOL, atomic);
        FUNCTION_LOG_PARAM(BOOL, truncate);
    FUNCTION_LOG_END();

    ASSERT(storage != NULL);
    ASSERT(file != NULL);
    ASSERT(modeFile != 0);
    ASSERT(modePath != 0);

    OBJ_NEW_BEGIN(StorageWritePosix, .childQty = MEM_CONTEXT_QTY_MAX, .callbackQty = 1)
    {
        *this = (StorageWritePosix)
        {
            .storage = storage,
            .directory = pathGetParent(file),
            .fd = -1,

            .interface = (StorageWriteInterface)
            {
                .type = STORAGE_POSIX_TYPE,
                .path = pathDup(file),
                .atomic = atomic,
                .createPath = createPath,
                .group = strDup(group),
                .modeFile = modeFile,
                .modePath = modePath,
                .syncFile = syncFile,
                .syncPath = syncPath,
                .truncate = truncate,
                .user = strDup(user),
                .timeModified = timeModified,

                .ioInterface = (IoWriteInterface)
                {
                    .close = storageWritePosixClose,
                    .fd = storageWritePosixFd,
                    .open = storageWritePosixOpen,
                    .write = storageWritePosix,
                },
            },
        };

        // Create temp file path
        if (atomic)
        {
            const String *const name = pathGetName(this->interface.path);

            this->fileTmp = pathSetNameFmt(pathDup(this->interface.path), "%s." STORAGE_FILE_TEMP_EXT, strZ(name));
        }
        else
            this->fileTmp = this->interface.path;
    }
    OBJ_NEW_END();

    FUNCTION_LOG_RETURN(STORAGE_WRITE, storageWriteNew(this, &this->interface));
}
