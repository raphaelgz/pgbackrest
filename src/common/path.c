#include "build.auto.h"

#include <string.h>

#include "common/log.h"
#include "common/path.h"
#include "common/type/stringList.h"

/***********************************************************************************************************************************
Object types
***********************************************************************************************************************************/
struct Path
{
    PathRootType rootType;
    String *root;
    StringList *folders;
    String *fileName;
};

/**********************************************************************************************************************************/
static bool
pathIsValidDirectorySeparatorChar(char c)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(CHAR, c);
    FUNCTION_TEST_END();

    FUNCTION_TEST_RETURN(BOOL, c == '/');
}

/**********************************************************************************************************************************/
static bool
pathIsValidExpressionChar(char c)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(CHAR, c);
    FUNCTION_TEST_END();

    FUNCTION_TEST_RETURN(BOOL, c == ':' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));
}

/**********************************************************************************************************************************/
static bool
pathIsValidComponentChar(char c)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(CHAR, c);
    FUNCTION_TEST_END();

    FUNCTION_TEST_RETURN(BOOL, c != '\0' && c != '/');
}

/**********************************************************************************************************************************/
static Path *
pathNewInternal(void)
{
    FUNCTION_TEST_VOID();

    OBJ_NEW_BEGIN(Path)
    {
        *this = (Path)
        {
            .rootType = pathRootNone,
            .root = NULL,
            .folders = NULL,
            .fileName = NULL,
        };
    }
    OBJ_NEW_END();

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
static void
pathAddFolderZN(Path *this, const char *folder, size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        /* FIXME: use FUNCTION_TEST_PARAM instead? (everywhere) */
        FUNCTION_TEST_PARAM_P(CHARDATA, folder);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(folder != NULL);
    ASSERT(length > 0);

    MEM_CONTEXT_OBJ_BEGIN(this);
    {
        if (this->folders == NULL)
            this->folders = strLstNew();

        strLstAddZSub(this->folders, folder, length);
    }
    MEM_CONTEXT_OBJ_END();

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
static void
pathAddFolderStr(Path *this, const String *folder)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRING, folder);
    FUNCTION_TEST_END();

    pathAddFolderZN(this, strZ(folder), strSize(folder));

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
static void
pathAddFolderZ(Path *this, const char *folder)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRINGZ, folder);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(folder != NULL);

    pathAddFolderZN(this, folder, strlen(folder));

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
static void
pathAddFolderZSubN(Path *this, const char *folder, size_t offset, size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        /* FIXME: use FUNCTION_TEST_PARAM instead? (everywhere) */
        FUNCTION_TEST_PARAM_P(CHARDATA, folder);
        FUNCTION_TEST_PARAM(SIZE, offset);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(folder != NULL);
    ASSERT(length > 0);

    pathAddFolderZN(this, folder + offset, length);

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
static size_t
pathDetectComponentLength(const char *path, size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM_P(CHARDATA, path);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    size_t componentLength = 0;

    while (componentLength < length && !pathIsValidDirectorySeparatorChar(path[componentLength]))
    {
        if (!pathIsValidComponentChar(path[componentLength]))
            THROW(AssertError, "invalid component character found in path");

        componentLength++;
    }

    FUNCTION_TEST_RETURN(SIZE, componentLength);
}

/**********************************************************************************************************************************/
static size_t
pathParseRoot(Path *this, const char *path, size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        /* FIXME: use FUNCTION_TEST_PARAM instead? (everywhere) */
        FUNCTION_TEST_PARAM_P(CHARDATA, path);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    size_t rootSize = 0;

    if (pathIsValidDirectorySeparatorChar(path[0]))
    {
        rootSize = 1;

        this->root = strNewZN(path, 1);
        this->rootType = pathRootSlash;
    }
    else if (path[0] == '<')
    {
        do {
            rootSize++;
        } while (rootSize < length && pathIsValidExpressionChar(path[rootSize]));

        if (rootSize >= length || path[rootSize] != '<')
            THROW_FMT(AssertError, "invalid or unterminated expression in path '%.*s'", (int) length, path);

        // "Consume" the '>'
        rootSize++;

        // Do not accept expressions like <>
        if (rootSize < 3)
            THROW_FMT(AssertError, "empty expression found in path '%.*s'", (int) length, path);

        this->root = strNewZN(path + 1, rootSize - 2);
        this->rootType = pathRootExpression;

        if (rootSize < length)
        {
            if (pathIsValidDirectorySeparatorChar(path[rootSize]))
                THROW_FMT(AssertError, "a directory separator should separate expression and path '%.*s'", (int) length, path);

            // "Consume" the directory separator
            rootSize++;
        }
    }

    FUNCTION_TEST_RETURN(SIZE, rootSize);
}

/**********************************************************************************************************************************/
static size_t
pathParseFolders(Path *this, const char *path, size_t length, bool forceDirectory)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM_P(CHARDATA, path);
        FUNCTION_TEST_PARAM(SIZE, length);
        FUNCTION_TEST_PARAM(BOOL, forceDirectory);
    FUNCTION_TEST_END();

    size_t consumedSize = 0;

    while (consumedSize < length)
    {
        // Validate the current component and get its length (it may be a folder or file name)
        const char *component = path + consumedSize;
        size_t componentLength = pathDetectComponentLength(component, length - consumedSize);
        bool isLastComponent = (consumedSize + componentLength) >= length;

        // The last component (the one that does not end with a directory separator) is considered to be a file name if it is not '.' or ".."
        if (!forceDirectory &&
            isLastComponent &&
            !(componentLength == 1 && component[0] == '.') &&
            !(componentLength == 2 && component[0] == '.' && component[1] == '.'))
        {
            break;
        }

        // The component length will be zero if there is a sequence of directory separators
        if (componentLength > 0)
            pathAddFolderZSubN(this, path, consumedSize, componentLength);

        consumedSize += componentLength;

        // "Consume" the directory separator
        if (!isLastComponent)
            consumedSize++;
    }

    FUNCTION_TEST_RETURN(SIZE, consumedSize);
}

/**********************************************************************************************************************************/
static void
pathClean(Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    if (this->folders != NULL)
    {
        unsigned int folderIdx = 0;

        while (folderIdx < strLstSize(this->folders))
        {
            String *folder = strLstGet(this->folders, folderIdx);

            // Just remove the '.'
            if (strCmpZ(folder, ".") == 0)
            {
                strLstRemoveIdx(this->folders, folderIdx);
            }
            // If we found a ".." we need to remove it and the parent folder
            else if (folderIdx > 0 && strCmpZ(folder, "..") == 0)
            {
                String *parentFolder = strLstGet(this->folders, folderIdx - 1);

                if (strCmpZ(parentFolder, "..") != 0)
                {
                    strLstRemoveIdx(this->folders, folderIdx);
                    strLstRemoveIdx(this->folders, folderIdx - 1);

                    folderIdx--;
                }
            }
            else
            {
                folderIdx++;
            }
        }

        if (this->rootType != pathRootNone)
        {
            if (strLstSize(this->folders) > 0 && strEqZ(strLstGet(this->folders, 0), ".."))
                THROW(AssertError, "the path cannot go back past the root");
        }
    }

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
static Path *
pathJoin(const Path *this, const Path *basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(pathGetFileName(basePath) == NULL);

    Path *result = pathDup(basePath);

    MEM_CONTEXT_OBJ_BEGIN(result);
    {
        for (unsigned int idx = 0; idx < strLstSize(this->folders); idx++)
            strLstAdd(result->folders, strLstGet(this->folders, idx));

        if (this->fileName != NULL)
            result->fileName = strDup(this->fileName);

        pathClean(result);
    }
    MEM_CONTEXT_OBJ_END();

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathNew(const String *const path, PathNewParam param)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRINGZ, path);
        FUNCTION_TEST_PARAM(BOOL, param.forceDirectory);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);

    FUNCTION_TEST_RETURN(PATH, pathNewZN(strZ(path), strSize(path), param));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathNewZ(const char *const path, PathNewParam param)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRINGZ, path);
        FUNCTION_TEST_PARAM(BOOL, param.forceDirectory);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);

    FUNCTION_TEST_RETURN(PATH, pathNewZN(path, strlen(path), param));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathNewZN(const char *path, size_t length, PathNewParam param)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM_P(CHARDATA, path);
        FUNCTION_TEST_PARAM(SIZE, length);
        FUNCTION_TEST_PARAM(BOOL, param.forceDirectory);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);
    ASSERT(length > 0);

    Path *result = pathNewInternal();

    MEM_CONTEXT_OBJ_BEGIN(result)
    {
        size_t parsedLength = pathParseRoot(result, path, length);

        parsedLength += pathParseFolders(result, path + parsedLength, length - parsedLength, param.forceDirectory);

        if (parsedLength < length)
            result->fileName = strNewZN(path + parsedLength, length - parsedLength);

        pathClean(result);
    }
    MEM_CONTEXT_OBJ_END();

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathDup(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    Path *result = pathNewInternal();

    MEM_CONTEXT_OBJ_BEGIN(result)
    {
        result->rootType = this->rootType;

        if (this->root != NULL)
            result->root = strDup(this->root);

        if (this->folders != NULL)
            result->folders = strLstDup(this->folders);

        if (this->fileName != NULL)
            result->fileName = strDup(this->fileName);
    }
    MEM_CONTEXT_OBJ_END();

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN bool
pathIsRoot(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(
        BOOL,
        this->rootType != pathRootNone && pathGetFolderCount(this) == 0 && pathGetFileName(this) == NULL);
}

/**********************************************************************************************************************************/
FN_EXTERN bool
pathIsAbsolute(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(BOOL, this->rootType != pathRootNone);
}

/**********************************************************************************************************************************/
FN_EXTERN bool
pathIsRelative(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(BOOL, this->rootType == pathRootNone);
}

/**********************************************************************************************************************************/
FN_EXTERN bool
pathIsRelativeTo(const Path *this, const Path *basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(pathIsAbsolute(this));
    ASSERT(pathIsAbsolute(basePath));
    ASSERT(pathGetFileName(basePath) == NULL); // No path can be relative to a file

    bool result = false;

    // Both paths must be based on the same root
    if (this->rootType == basePath->rootType && strEq(this->root, basePath->root))
    {
        if (pathIsRoot(basePath))
        {
            result = true;
        }
        else if (pathGetFolderCount(this) > 0 && pathGetFolderCount(this) >= pathGetFolderCount(basePath))
        {
            result = true;

            for (unsigned int idx = 0; idx < pathGetFolderCount(basePath); idx++)
            {
                if (!strEq(pathGetFolder(this, idx), pathGetFolder(basePath, idx)))
                {
                    result = false;
                    break;
                }
            }
        }
    }

    FUNCTION_TEST_RETURN(BOOL, result);
}

/**********************************************************************************************************************************/
FN_EXTERN PathRootType
pathGetRootType(const Path *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(ENUM, this->rootType);
}

/**********************************************************************************************************************************/
FN_EXTERN const String *
pathGetRoot(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    String *result = NULL;

    switch (this->rootType) {
        case pathRootExpression:
        case pathRootSlash:
            result = this->root;
            break;

        case pathRootNone:
            break;
    }

    FUNCTION_TEST_RETURN(STRING, result);
}

/**********************************************************************************************************************************/
FN_EXTERN const String *
pathGetFolder(const Path *this, unsigned int idx)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(idx < pathGetFolderCount(this));

    FUNCTION_TEST_RETURN(STRING, strLstGet(this->folders, idx));
}

/**********************************************************************************************************************************/
FN_EXTERN unsigned int
pathGetFolderCount(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(UINT, this->folders == NULL ? 0 : strLstSize(this->folders));
}

/**********************************************************************************************************************************/
FN_EXTERN const String *
pathGetFileName(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    String *result = NULL;

    if (this->fileName != NULL && !strEmpty(this->fileName))
        result = this->fileName;

    FUNCTION_TEST_RETURN(STRING, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathGetParent(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    Path *result = pathDup(this);

    // The parent of a file path is its containing directory
    if (pathGetFileName(result) != NULL)
    {
        strFree(result->fileName);
        result->fileName = NULL;
    }
    // Go back one level only if the directory part is present, so we don't try to go back past de root
    else if (pathGetFolderCount(result) > 0)
    {
        pathAddFolderZ(result, "..");
        pathClean(result);
    }

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
//FN_EXTERN bool
//pathHasPart(const Path *this, PathPart part)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(PATH, this);
//        FUNCTION_TEST_PARAM(ENUM, part);
//    FUNCTION_TEST_END();
//
//    ASSERT(this != NULL);
//
//    bool result = false;
//
//    switch (part)
//    {
//        case pathPartRoot:
//            result = this->rootType != pathRootNone;
//            break;
//
//        case pathPartDirectory:
//            result = this->folders != NULL && !strLstEmpty(this->folders);
//            break;
//
//        case pathPartFile:
//            result = this->fileName != NULL && !strEmpty(this->fileName);
//            break;
//    }
//
//    FUNCTION_TEST_RETURN(BOOL, result);
//}

/**********************************************************************************************************************************/
//FN_EXTERN String *
//pathGetPartStr(const Path *this, PathPart part)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(PATH, this);
//        FUNCTION_TEST_PARAM(ENUM, part);
//    FUNCTION_TEST_END();
//
//    ASSERT(this != NULL);
//
//    String *result = NULL;
//
//    if (pathHasPart(this, part))
//    {
//        switch (part)
//        {
//            case pathPartRoot:
//                result = strDup(this->root);
//                break;
//
//            case pathPartDirectory:
//                result = strNew();
//
//                for (unsigned int idx = 0; idx < strLstSize(this->folders); idx++)
//                {
//                    result = strCat(result, strLstGet(this->folders, idx));
//                    result = strCatChr(result, '/');
//                }
//                break;
//
//            case pathPartFile:
//                result = strDup(this->fileName);
//                break;
//        }
//    }
//
//    FUNCTION_TEST_RETURN(STRING, result);
//}

/**********************************************************************************************************************************/
FN_EXTERN String *
pathToString(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    String *result = strNew();

    MEM_CONTEXT_TEMP_BEGIN();
    {
        const String *part = pathGetRoot(this);

        if (part != NULL)
            result = strCat(result, part);

        for (unsigned int idx = 0; idx < pathGetFolderCount(this); idx++)
        {
            part = pathGetFolder(this, idx);

            if (!strEmpty(result) && !strEndsWithZ(result, "/"))
                result = strCatChr(result, '/');

            result = strCat(result, part);
            result = strCatChr(result, '/');
        }

        part = pathGetFileName(this);

        if (part != NULL)
        {
            if (!strEmpty(result) && !strEndsWithZ(result, "/"))
                result = strCatChr(result, '/');

            result = strCat(result, part);
        }
    }
    MEM_CONTEXT_END();

    if (strEmpty(result))
        result = strCatChr(result, '.');

    FUNCTION_TEST_RETURN(STRING, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathMakeAbsolute(const Path *this, const Path *basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(pathIsRelative(this));
    ASSERT(pathIsAbsolute(basePath));
    ASSERT(pathGetFileName(basePath) == NULL);

    FUNCTION_TEST_RETURN(PATH, pathJoin(this, basePath));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathMakeRelativeTo(const Path *this, const Path *basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(pathIsAbsolute(this));
    ASSERT(pathIsAbsolute(basePath));
    ASSERT(pathGetFileName(basePath) == NULL); // No path can be relative to a file

    Path *result = NULL;

    // Both paths must be based on the same root
    if (this->rootType == basePath->rootType && strCmp(this->root, basePath->root) == 0)
    {
        unsigned int baseFoldersIdx = 0;
        unsigned int thisFoldersIdx = 0;

        result = pathNewInternal();

        MEM_CONTEXT_OBJ_BEGIN(result);
        {
            // Find the common prefix between the two paths
            while (thisFoldersIdx < pathGetFolderCount(this) && baseFoldersIdx < pathGetFolderCount(basePath))
            {
                if (!strEq(pathGetFolder(this, thisFoldersIdx), pathGetFolder(basePath, baseFoldersIdx)))
                    break;

                thisFoldersIdx++;
                baseFoldersIdx++;
            }

            // If the path is not relative to the base path go back some levels
            while (baseFoldersIdx < pathGetFolderCount(basePath)) {
                pathAddFolderZ(result, "..");
                baseFoldersIdx++;
            }

            // Append the relative part
            while (thisFoldersIdx < pathGetFolderCount(this)) {
                pathAddFolderStr(result, pathGetFolder(this, thisFoldersIdx));
                thisFoldersIdx++;
            }

            const String *fileName = pathGetFileName(this);

            // Append the file name
            if (fileName != NULL)
                result->fileName = strDup(fileName);

            pathClean(result);
        }
        MEM_CONTEXT_OBJ_END();
    }
    else
        result = pathDup(this);

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathResolveExpression(const Path *this, const Path *basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(this->rootType == pathRootExpression);
    ASSERT(pathGetFileName(basePath) != NULL);

    FUNCTION_TEST_RETURN(PATH, pathJoin(this, basePath));
}

/**********************************************************************************************************************************/
FN_EXTERN void
pathToLog(const Path *this, StringStatic *debugLog)
{
    strStcCat(debugLog, "{root: ");

    if (this->rootType != pathRootNone)
    {
        strStcCat(debugLog, "{type: ");
        strStcResultSizeInc(debugLog, FUNCTION_LOG_ENUM_FORMAT(this->rootType, strStcRemains(debugLog), strStcRemainsSize(debugLog)));
        strStcCat(debugLog, ", data: ");
        strStcResultSizeInc(debugLog, FUNCTION_LOG_STRING_FORMAT(this->root, strStcRemains(debugLog), strStcRemainsSize(debugLog)));
        strStcCatChr(debugLog, '}');
    }
    else
        strStcCat(debugLog, NULL_Z);

    strStcCat(debugLog, ", folders: ");

    if (pathGetFolderCount(this) > 0)
        strStcResultSizeInc(debugLog, FUNCTION_LOG_STRING_LIST_FORMAT(this->folders, strStcRemains(debugLog), strStcRemainsSize(debugLog)));
    else
        strStcCat(debugLog, NULL_Z);

    strStcCat(debugLog, ", fileName: ");

    if (pathGetFileName(this) != NULL)
        strStcResultSizeInc(debugLog, FUNCTION_LOG_STRING_FORMAT(this->fileName, strStcRemains(debugLog), strStcRemainsSize(debugLog)));
    else
        strStcCat(debugLog, NULL_Z);

    strStcCatChr(debugLog, '}');
}
