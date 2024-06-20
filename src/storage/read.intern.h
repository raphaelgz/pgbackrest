/***********************************************************************************************************************************
Storage Read Interface Internal
***********************************************************************************************************************************/
#ifndef STORAGE_READ_INTERN_H
#define STORAGE_READ_INTERN_H

#include "common/path.h"
#include "common/io/read.h"

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
typedef struct StorageReadInterface
{
    StringId type;                                                  // Storage type
    const Path *path;
    bool compressible;                                              // Is this file compressible?
    unsigned int compressLevel;                                     // Level to use for compression
    bool ignoreMissing;
    uint64_t offset;                                                // Where to start reading in the file
    const Variant *limit;                                           // Limit how many bytes are read (NULL for no limit)
    IoReadInterface ioInterface;
} StorageReadInterface;

#include "storage/read.h"

FN_EXTERN StorageRead *storageReadNew(void *driver, const StorageReadInterface *interface);

#endif
