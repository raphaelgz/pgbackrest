/***********************************************************************************************************************************
S3 Storage File Write
***********************************************************************************************************************************/
#ifndef STORAGE_S3_WRITE_H
#define STORAGE_S3_WRITE_H

#include "storage/s3/storage.intern.h"
#include "storage/write.h"

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN StorageWrite *storageWriteS3New(StorageS3 *storage, const Path *file, size_t partSize);

#endif
