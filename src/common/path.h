/***********************************************************************************************************************************
Path Handler
***********************************************************************************************************************************/
#ifndef COMMON_PATH_H
#define COMMON_PATH_H

#include "common/type/string.h"

FN_EXTERN bool pathIsAbsolute(const String *path);
FN_EXTERN bool pathIsRelative(const String *path);
FN_EXTERN bool pathIsRelativeTo(const String *basePath, const String *path);
FN_EXTERN String *pathMakeAbsolute(const String *basePath, const String *path);
FN_EXTERN String *pathMakeRelativeTo(const String *basePath, const String *path);

#endif
