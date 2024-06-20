/***********************************************************************************************************************************
CIFS Storage
***********************************************************************************************************************************/
#ifndef STORAGE_CIFS_STORAGE_H
#define STORAGE_CIFS_STORAGE_H

#include "storage/storage.h"

/***********************************************************************************************************************************
Storage type
***********************************************************************************************************************************/
#define STORAGE_CIFS_TYPE                                           STRID5("cifs", 0x999230)

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN Storage *storageCifsNew(
    const Path *path, mode_t modeFile, mode_t modePath, bool write, StoragePathExpressionCallback pathExpressionFunction);

#endif
