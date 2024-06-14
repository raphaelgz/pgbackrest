/***********************************************************************************************************************************
Path Handler
***********************************************************************************************************************************/
#ifndef COMMON_PATH_H
#define COMMON_PATH_H

#include "common/type/string.h"

/***********************************************************************************************************************************
Path object
***********************************************************************************************************************************/
typedef struct Path Path;

//typedef enum
//{
//    // The "var/lib/postgresql/16/main" part of "/var/lib/postgresql/16/main/PG_VERSION"
//    pathPartDirectory,
//
//    // The "PG_VERSION" part of "/var/lib/postgresql/16/main/PG_VERSION"
//    pathPartFile,
//
//    // The first "/" in "/var/lib/postgresql/16/main/PG_VERSION"
//    pathPartRoot,
//} PathPart;

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
typedef struct PathNewParam
{
    VAR_PARAM_HEADER;
    bool forceDirectory;
} PathNewParam;

#define pathNewP(path, ...) pathNew(path, (PathNewParam){VAR_PARAM_INIT, __VA_ARGS__})
#define pathNewZP(path, ...) pathNewZ(path, (PathNewParam){VAR_PARAM_INIT, __VA_ARGS__})
#define pathNewZNP(path, length, ...) pathNewZN(path, length, (PathNewParam){VAR_PARAM_INIT, __VA_ARGS__})

FN_EXTERN Path *pathNew(const String *path, PathNewParam param);
FN_EXTERN Path *pathNewZ(const char *path, PathNewParam param);
FN_EXTERN Path *pathNewZN(const char *path, size_t length, PathNewParam param);
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
FN_EXTERN const String *pathGetFolder(const Path *this, unsigned int idx);
FN_EXTERN unsigned int pathGetFolderCount(const Path *this);
FN_EXTERN const String *pathGetFileName(const Path *this);
FN_EXTERN Path *pathGetParent(const Path *this);
//FN_EXTERN bool pathHasPart(const Path *this, PathPart part);
//FN_EXTERN String *pathGetPartStr(const Path *this, PathPart part);

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
FN_EXTERN String *pathToString(const Path *this);
//FN_EXTERN Path *pathChangeDir(Path *this, const Path *path);
//FN_EXTERN Path *pathChangeDirStr(Path *this, const String *path);
//FN_EXTERN Path *pathChangeDirZ(Path *this, const char *path);
//FN_EXTERN Path *pathChangeDirZN(Path *this, const char *path, size_t length);
//FN_EXTERN FN_PRINTF(2, 3) Path *pathChangeDirFmt(Path *this, const char *format, ...);
FN_EXTERN Path *pathMakeAbsolute(const Path *this, const Path *basePath);
FN_EXTERN Path *pathMakeRelativeTo(const Path *this, const Path *basePath);
FN_EXTERN Path *pathResolveExpression(const Path *this, const Path *basePath);

/***********************************************************************************************************************************
Log support
***********************************************************************************************************************************/
FN_EXTERN void pathToLog(const Path *this, StringStatic *debugLog);

#define FUNCTION_LOG_PATH_TYPE                                                                                                     \
    Path *
#define FUNCTION_LOG_PATH_FORMAT(value, buffer, bufferSize)                                                                        \
    FUNCTION_LOG_OBJECT_FORMAT(value, pathToLog, buffer, bufferSize)

#endif
