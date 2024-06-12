#include "build.auto.h"

#include <string.h>

#include "common/path.h"
#include "common/type/stringList.h"

/***********************************************************************************************************************************
Object types
***********************************************************************************************************************************/
typedef enum
{
    pathRootNone,
    pathRootSlash,
    pathRootExpression,
//    pathRootDrive,
//    pathRootUNC,
} PathRootType;

struct Path
{
    PathRootType rootType;
    String *root;
    StringList *folders;
    String *fileName;
};
#include "common/log.h"
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
    FUNCTION_TEST_BEGIN();
    FUNCTION_TEST_END();

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
        {
            if (this->folders == NULL)
                this->folders = strLstNew();

            strLstAddZSubN(this->folders, path, consumedSize, componentLength);
        }

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

    if (pathHasPart(this, pathPartDirectory))
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
    }

    FUNCTION_TEST_RETURN_VOID();
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
FN_EXTERN bool
pathIsRoot(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(
        BOOL,
        pathHasPart(this, pathPartRoot) && !pathHasPart(this, pathPartDirectory) && !pathHasPart(this, pathPartFile));
}

/**********************************************************************************************************************************/
FN_EXTERN bool
pathIsAbsolute(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(BOOL, pathHasPart(this, pathPartRoot));
}

/**********************************************************************************************************************************/
FN_EXTERN bool
pathIsRelative(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(BOOL, !pathHasPart(this, pathPartRoot));
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
    ASSERT(!pathHasPart(basePath, pathPartFile)); // No path can be relative to a file

    bool result = false;

    // Both paths must be based on the same root
    if (this->rootType == basePath->rootType && strCmp(this->root, basePath->root) == 0)
    {
        if (!pathHasPart(basePath, pathPartDirectory))
        {
            result = true;
        }
        else if (pathHasPart(this, pathPartDirectory) && strLstSize(this->folders) >= strLstSize(basePath->folders))
        {
            result = true;

            for (unsigned int idx = 0; idx < strLstSize(basePath->folders); idx++)
            {
                if (strCmp(strLstGet(this->folders, idx), strLstGet(basePath->folders, idx)) != 0)
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
FN_EXTERN bool
pathHasPart(const Path *this, PathPart part)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(ENUM, part);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    bool result = false;

    switch (part)
    {
        case pathPartRoot:
            result = this->rootType != pathRootNone;
            break;

        case pathPartDirectory:
            result = this->folders != NULL && !strLstEmpty(this->folders);
            break;

        case pathPartFile:
            result = this->fileName != NULL && !strEmpty(this->fileName);
            break;
    }

    FUNCTION_TEST_RETURN(BOOL, result);
}

/**********************************************************************************************************************************/
FN_EXTERN String *
pathGetPartStr(const Path *this, PathPart part)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(ENUM, part);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    String *result = NULL;

    if (pathHasPart(this, part))
    {
        switch (part)
        {
            case pathPartRoot:
                result = strDup(this->root);
                break;

            case pathPartDirectory:
                for (unsigned int idx = 0; idx < strLstSize(this->folders); idx++)
                {
                    if (result == NULL)
                        result = strNew();
                    else
                        result = strCatChr(result, '/');

                    result = strCat(result, strLstGet(this->folders, idx));
                }
                break;

            case pathPartFile:
                result = strDup(this->fileName);
                break;
        }
    }

    FUNCTION_TEST_RETURN(STRING, result);
}

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
        String *part = pathGetPartStr(this, pathPartRoot);

        if (part != NULL)
            result = strCat(result, part);

        part = pathGetPartStr(this, pathPartDirectory);

        if (part != NULL)
            result = strCat(result, part);

        part = pathGetPartStr(this, pathPartFile);

        if (part != NULL)
        {
            if (!strEmpty(result))
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
    ASSERT(!pathHasPart(basePath, pathPartFile));

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
    ASSERT(!pathHasPart(basePath, pathPartFile)); // No path can be relative to a file

    Path *result = pathDup(this);

    // Both paths must be based on the same root
    if (this->rootType == basePath->rootType && strCmp(this->root, basePath->root) == 0)
    {
        strFree(result->root);

        result->root = NULL;
        result->rootType = pathRootNone;

        if (pathHasPart(this, pathPartDirectory) && pathHasPart(basePath, pathPartDirectory))
        {
            
        }
//        else if (pathHasPart(this, pathPartDirectory) && strLstSize(this->folders) >= strLstSize(basePath->folders))
//        {
//            result = true;
//
//            for (unsigned int idx = 0; idx < strLstSize(basePath->folders); idx++)
//            {
//                if (strCmp(strLstGet(this->folders, idx), strLstGet(basePath->folders, idx)) != 0)
//                {
//                    result = false;
//                    break;
//                }
//            }
//        }
    }

}

/**********************************************************************************************************************************/
//FN_EXTERN bool
//pathIsAbsolute(const String *const path)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(STRING, path);
//    FUNCTION_TEST_END();
//
//    ASSERT(path != NULL);
//
//    FUNCTION_TEST_RETURN(BOOL, strSize(path) >= 1 && strZ(path)[0] == '/');
//}
//
///**********************************************************************************************************************************/
//FN_EXTERN bool
//pathIsRelative(const String *const path)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(STRING, path);
//    FUNCTION_TEST_END();
//
//    ASSERT(path != NULL);
//
//    FUNCTION_TEST_RETURN(BOOL, !pathIsAbsolute(path));
//}
//
///**********************************************************************************************************************************/
//FN_EXTERN bool
//pathIsRelativeTo(const String *const basePath, const String *const path)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(STRING, basePath);
//        FUNCTION_TEST_PARAM(STRING, path);
//    FUNCTION_TEST_END();
//
//    ASSERT(basePath != NULL && pathIsAbsolute(basePath));
//    ASSERT(path != NULL && pathIsAbsolute(path));
//
//    bool result =
//        strEqZ(basePath, "/") ||
//        (strBeginsWith(path, basePath) &&
//        (strSize(basePath) == strSize(path) || *(strZ(path) + strSize(basePath)) == '/'));
//
//    FUNCTION_TEST_RETURN(BOOL, result);
//}
//
///**********************************************************************************************************************************/
//FN_EXTERN String *
//pathMakeAbsolute(const String *const basePath, const String *const path)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(STRING, basePath);
//        FUNCTION_TEST_PARAM(STRING, path);
//    FUNCTION_TEST_END();
//
//    ASSERT(basePath != NULL && pathIsAbsolute(basePath));
//    ASSERT(path != NULL);
//
//    String *result = NULL;
//
//    if (pathIsRelative(path))
//    {
//        if (strEqZ(basePath, "/"))
//            result = strNewFmt("/%s", strZ(path));
//        else
//            result = strNewFmt("%s/%s", strZ(basePath), strZ(path));
//    }
//    else
//        result = strDup(path);
//
//    FUNCTION_TEST_RETURN(STRING, result);
//}
//
///**********************************************************************************************************************************/
//FN_EXTERN String *
//pathMakeRelativeTo(const String *basePath, const String *path)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(STRING, basePath);
//        FUNCTION_TEST_PARAM(STRING, path);
//    FUNCTION_TEST_END();
//
//    ASSERT(basePath != NULL && pathIsAbsolute(basePath));
//    ASSERT(path != NULL && pathIsAbsolute(path));
//
//    String *result = strNew();
//
//    MEM_CONTEXT_TEMP_BEGIN();
//    {
//        StringList *baseComponents = strLstNewSplitZ(basePath, "/");
//        StringList *pathComponents = strLstNewSplitZ(path, "/");
//        unsigned int baseComponentsIdx = 1;
//        unsigned int baseComponentsSize = strLstSize(baseComponents);
//        unsigned int pathComponentsIdx = 1;
//        unsigned int pathComponentsSize = strLstSize(pathComponents);
//
//        // Find the common prefix between the two paths
//        while (baseComponentsIdx < baseComponentsSize && pathComponentsIdx < pathComponentsSize)
//        {
//            if (!strEq(strLstGet(baseComponents, baseComponentsIdx), strLstGet(pathComponents, pathComponentsIdx)))
//                break;
//
//            baseComponentsIdx++;
//            pathComponentsIdx++;
//        }
//
//        // If the path is not relative to the base path go back some levels
//        for (unsigned int idx = baseComponentsIdx; idx < baseComponentsSize; idx++)
//            strCatZ(result, "../");
//
//        // Append the relative part
//        for (unsigned int idx = pathComponentsIdx; idx < pathComponentsSize; idx++)
//        {
//            strCat(result, strLstGet(pathComponents, idx));
//
//            if ((idx + 1) != pathComponentsSize)
//                strCatZN(result, "/", 1);
//        }
//
//        // If both paths are equal, return the base path
//        if (strEmpty(result))
//            strCat(result, basePath);
//    }
//    MEM_CONTEXT_TEMP_END();
//
//    FUNCTION_TEST_RETURN(STRING, result);
//}
