/***********************************************************************************************************************************
Path Handler
***********************************************************************************************************************************/
#ifndef COMMON_PATH_H
#define COMMON_PATH_H

#include "common/type/string.h"
#include "common/type/stringList.h"

/***********************************************************************************************************************************
Path object
***********************************************************************************************************************************/
typedef struct Path Path;

typedef enum
{
    // No root
    pathRootNone,

    // The root is '/'
    pathRootSlash,

    // The root is an expression, like "<EXP>"
    pathRootExpression,
} PathRootType;

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
FN_EXTERN Path *pathNew(const String *path);
FN_EXTERN Path *pathNewZ(const char *path);
FN_EXTERN Path *pathNewZN(const char *path, size_t length);
FN_EXTERN FN_PRINTF(1, 2) Path *pathNewFmt(const char *format, ...);
FN_EXTERN Path *pathNewAbsolute(const String *path, const Path *basePath);
FN_EXTERN Path *pathNewAbsoluteZ(const char *path, const Path *basePath);
FN_EXTERN Path *pathNewAbsoluteZN(const char *path, size_t length, const Path *basePath);
FN_EXTERN Path *pathDup(const Path *this);

/***********************************************************************************************************************************
Destructor
***********************************************************************************************************************************/
FN_INLINE_ALWAYS void
pathFree(Path *const this)
{
    objFree(this);
}

/***********************************************************************************************************************************
Getters/Setters
***********************************************************************************************************************************/
FN_EXTERN bool pathIsRoot(const Path *this);
FN_EXTERN bool pathIsAbsolute(const Path *this);
FN_EXTERN bool pathIsRelative(const Path *this);
FN_EXTERN bool pathIsRelativeTo(const Path *this, const Path *basePath);
FN_EXTERN PathRootType pathGetRootType(const Path *this);
FN_EXTERN const String *pathGetRoot(const Path *this);
FN_EXTERN bool pathHasName(const Path *this);
FN_EXTERN const String *pathGetName(const Path *this);
FN_EXTERN Path *pathSetName(Path *this, const String *name);
FN_EXTERN Path *pathSetNameZ(Path *this, const char *name);
FN_EXTERN Path *pathSetNameZN(Path *this, const char *name, size_t length);
FN_EXTERN FN_PRINTF(2, 3) Path *pathSetNameFmt(Path *this, const char *format, ...);
FN_EXTERN const String *pathGetComponent(const Path *this, unsigned int index);
FN_EXTERN unsigned int pathGetComponentCount(const Path *this);
FN_EXTERN Path *pathAppendComponent(Path *this, const String *component);
FN_EXTERN Path *pathAppendComponentZ(Path *this, const char *component);
FN_EXTERN Path *pathAppendComponentZN(Path *this, const char *component, size_t length);
FN_EXTERN Path *pathGetParent(const Path *this);

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
FN_EXTERN bool pathEq(const Path *this, const Path *compare);
FN_EXTERN const String *pathStr(const Path *this);
FN_EXTERN Path *pathJoin(Path *this, const Path *basePath);
FN_EXTERN Path *pathMakeAbsolute(Path *this, const Path *basePath);
FN_EXTERN Path *pathMakeRelativeTo(Path *this, const Path *basePath);
FN_EXTERN Path *pathResolveExpression(const Path *this, const Path *basePath);
FN_EXTERN Path *pathResolveExpressionStr(const Path *this, const String *basePath);
FN_EXTERN Path *pathResolveExpressionZ(const Path *this, const char *basePath);
FN_EXTERN Path *pathResolveExpressionZN(const Path *this, const char *basePath, size_t length);
FN_EXTERN FN_PRINTF(2, 3) Path *pathResolveExpressionFmt(const Path *this, const char *format, ...);

FN_INLINE_ALWAYS const char *
pathZ(const Path *const this)
{
    return strZ(pathStr(this));
}

/***********************************************************************************************************************************
Log support
***********************************************************************************************************************************/
FN_EXTERN void pathToLog(const Path *this, StringStatic *debugLog);

#define FUNCTION_LOG_PATH_TYPE                                                                                                     \
    Path *
#define FUNCTION_LOG_PATH_FORMAT(value, buffer, bufferSize)                                                                        \
    FUNCTION_LOG_OBJECT_FORMAT(value, pathToLog, buffer, bufferSize)

#endif
