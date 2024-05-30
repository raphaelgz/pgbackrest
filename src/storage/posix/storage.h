/***********************************************************************************************************************************
Posix Storage
***********************************************************************************************************************************/
#ifndef STORAGE_POSIX_STORAGE_H
#define STORAGE_POSIX_STORAGE_H

#include "storage/storage.h"

/***********************************************************************************************************************************
Storage type
***********************************************************************************************************************************/
#define STORAGE_POSIX_TYPE                                          STRID5("posix", 0x184cdf00)

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
typedef struct StoragePosixNewParam
{
    VAR_PARAM_HEADER;
    bool write;
    mode_t modeFile;
    mode_t modePath;
} StoragePosixNewParam;

#define storagePosixNewP(path, ...)                                                                                                \
    storagePosixNew(path, (StoragePosixNewParam){VAR_PARAM_INIT, __VA_ARGS__})

#define storagePosixNewStrP(path, ...)                                                                                                \
    storagePosixNewStr(path, (StoragePosixNewParam){VAR_PARAM_INIT, __VA_ARGS__})

FN_EXTERN Storage *storagePosixNew(const Path *path, StoragePosixNewParam param);
FN_EXTERN Storage *storagePosixNewStr(const String *path, StoragePosixNewParam param);

#endif
