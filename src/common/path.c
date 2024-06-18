#include "build.auto.h"

#include <string.h>

#include "common/log.h"
#include "common/path.h"
#include "common/type/list.h"
//#include "common/type/stringList.h"

/***********************************************************************************************************************************
Object types
***********************************************************************************************************************************/

typedef struct PathComponentInfo
{
    size_t offset;
    size_t length;
} PathComponentInfo;

struct Path
{
    PathRootType rootType;
    String *str;
    List *componentsInfo;
//    StringList *components;
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
            .str = NULL,
            .componentsInfo = NULL,
//            .components = NULL,
        };
    }
    OBJ_NEW_END();

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
static Path *
pathInsertComponentZN(Path *const this, const unsigned int componentIndex, const char *const component, const size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(UINT, componentIndex);
        FUNCTION_TEST_PARAM(CHARDATA, component);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);
    ASSERT(length > 0);

    MEM_CONTEXT_OBJ_BEGIN(this);
    {
        if (this->componentsInfo == NULL)
            this->componentsInfo = lstNewP(sizeof(PathComponentInfo), .sortOrder = sortOrderNone);

        if (this->str == NULL)
            this->str = strNew();

        bool addSeparatorAfter = false;
        bool addSeparatorBefore = false;
        PathComponentInfo *componentInfo = NULL;

        // Using a block only to make harder to use the nextComponentInfo and previousComponentInfo variables after the call to lstInsert()
        {
            PathComponentInfo *nextComponentInfo = NULL;
            PathComponentInfo *previousComponentInfo = NULL;

            if (componentIndex < lstSize(this->componentsInfo))
                nextComponentInfo = (PathComponentInfo *) lstGet(this->componentsInfo, componentIndex);

            if (componentIndex > 0 && componentIndex <= lstSize(this->componentsInfo))
                previousComponentInfo = (PathComponentInfo *) lstGet(this->componentsInfo, componentIndex - 1);

            // If the new component is not the first, calculate the offset based on the previous component
            if (previousComponentInfo != NULL)
            {
                PathComponentInfo newComponentInfo =
                {
                    .offset = previousComponentInfo->offset + previousComponentInfo->length,
                    .length = length,
                };

                // Ensure that the new component is separated from the previous one with a directory separator
                if (!pathIsValidDirectorySeparatorChar(strZ(this->str)[newComponentInfo.offset - 1]))
                    addSeparatorBefore = true;
                // Ensure that the new component is separated from the next one with a directory separator
                else if (nextComponentInfo != NULL)
                    addSeparatorAfter = true;

                componentInfo = (PathComponentInfo *) lstInsert(this->componentsInfo, componentIndex, &newComponentInfo);
            }
            else
            {
                // Ensure that the new component is separated from the next one with a directory separator
                if (nextComponentInfo != NULL && !pathIsValidDirectorySeparatorChar(component[length - 1]))
                {
                    if (nextComponentInfo->offset == 0 || !pathIsValidDirectorySeparatorChar(strZ(this->str)[nextComponentInfo->offset - 1]))
                        addSeparatorAfter = true;
                }

                PathComponentInfo newComponentInfo =
                {
                    .offset = 0,
                    .length = length,
                };

                componentInfo = (PathComponentInfo *) lstInsert(this->componentsInfo, componentIndex, &newComponentInfo);
            }
        }

        // Update the path string
        if (addSeparatorAfter)
            strInsert(this->str, componentInfo->offset, FSLASH_STR);

        strInsertZN(this->str, componentInfo->offset, component, length);

        if (addSeparatorAfter)
        {
            strInsert(this->str, componentInfo->offset, FSLASH_STR);
            componentInfo->offset++;
        }

        // Update the offset of all components that come after the new component
        for (unsigned int idx = componentIndex + 1; idx < lstSize(this->componentsInfo); idx++)
        {
            PathComponentInfo *info = (PathComponentInfo *) lstGet(this->componentsInfo, idx);

            info->offset += componentInfo->length;

            if (addSeparatorAfter)
                info->offset++;

            if (addSeparatorBefore)
                info->offset++;
        }
    }
    MEM_CONTEXT_OBJ_END();

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
static Path *
pathRemoveComponent(Path *const this, const unsigned int componentIndex)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(UINT, componentIndex);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(this->componentsInfo != NULL && componentIndex < lstSize(this->componentsInfo));

    PathComponentInfo *componentInfo = (PathComponentInfo *) lstGet(this->componentsInfo, componentIndex);
    const size_t componentEndStringIndex = componentInfo->offset + componentInfo->length;

    // Remove the directory separator if it is not part of the component
    if (!pathIsValidDirectorySeparatorChar(strZ(this->str)[componentEndStringIndex - 1]))
    {
        if (componentEndStringIndex < strSize(this->str) && pathIsValidDirectorySeparatorChar(strZ(this->str)[componentEndStringIndex]))
            componentInfo->length++; // Increase the length in order to "magically" remove the directory separator without any additional checks
    }

    // Update the offset of all components that come after the removed component
    for (unsigned int idx = componentIndex + 1; idx < lstSize(this->componentsInfo); idx++)
    {
        PathComponentInfo *info = (PathComponentInfo *) lstGet(this->componentsInfo, idx);

        info->offset -= componentInfo->length;
    }

    strRemove(this->str, componentInfo->offset, componentInfo->length);
    lstRemoveIdx(this->componentsInfo, componentIndex);

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
static Path *
pathReplaceComponentZN(Path *const this, const unsigned int componentIndex, const char *const component, const size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(UINT, componentIndex);
        FUNCTION_TEST_PARAM(CHARDATA, component);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(this->componentsInfo != NULL && componentIndex < lstSize(this->componentsInfo));
    ASSERT(component != NULL);
    ASSERT(length > 0);

    pathRemoveComponent(this, componentIndex);

    FUNCTION_TEST_RETURN(PATH, pathInsertComponentZN(this, componentIndex, component, length));
}

/**********************************************************************************************************************************/
static Path *
pathSetComponentLength(Path *const this, const unsigned int componentIndex, const size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(UINT, componentIndex);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    PathComponentInfo *info = (PathComponentInfo *) lstGet(this->componentsInfo, componentIndex);

    size_t oldLength = info->length;

    info->length = length;

    for (unsigned int idx = componentIndex + 1; idx < lstSize(this->componentsInfo); idx++)
    {
        info = (PathComponentInfo *) lstGet(this->componentsInfo, idx);

        if (oldLength < length)
            info->offset += length - oldLength;
        else
            info->offset -= oldLength - length;
    }

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
static Path *
pathSetRootComponentZN(Path *this, PathRootType rootType, const char *root, size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(ENUM, rootType);
        /* FIXME: use FUNCTION_TEST_PARAM instead? (everywhere) */
        FUNCTION_TEST_PARAM_P(CHARDATA, root);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(root != NULL || length == 0);

    if (root == NULL)
        root = "";

    MEM_CONTEXT_OBJ_BEGIN(this);
    {
        PathComponentInfo *rootComponentInfo = NULL;

        if (this->componentsInfo == NULL)
        {
            PathComponentInfo newInfo =
            {
                .offset = 0,
                .length = 0
            };

            this->componentsInfo = lstNewP(sizeof(PathComponentInfo), .sortOrder = sortOrderNone);

            rootComponentInfo = lstAdd(this->componentsInfo, &newInfo);
        }
        else
            rootComponentInfo = lstGet(this->componentsInfo, 0);

        if (this->str == NULL)
            this->str = strNew();

        this->rootType = rootType;

        strReplaceZN(this->str, rootComponentInfo->offset, rootComponentInfo->length, root, length);

        pathSetComponentLength(this, 0, length);
    }
    MEM_CONTEXT_OBJ_END();

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
static Path *
pathSetRootComponentStr(Path *const this, PathRootType rootType, const String *const root)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(ENUM, rootType);
        FUNCTION_TEST_PARAM(STRING, root);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(root != NULL);

    FUNCTION_TEST_RETURN(PATH, pathSetRootComponentZN(this, rootType, strZ(root), strSize(root)));
}

/**********************************************************************************************************************************/
static Path *
pathAppendNonRootComponentZN(Path *const this, const char *const component, const size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        /* FIXME: use FUNCTION_TEST_PARAM instead? (everywhere) */
        FUNCTION_TEST_PARAM_P(CHARDATA, component);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);
    ASSERT(length > 0);

    if (this->componentsInfo == NULL || lstEmpty(this->componentsInfo))
        pathSetRootComponentZN(this, pathRootNone, NULL, 0);

    if (this->rootType != pathRootNone && !strEndsWith(this->str, FSLASH_STR))
        strCat(this->str, FSLASH_STR);

    PathComponentInfo newInfo =
    {
        .offset = strSize(this->str),
        .length = length
    };

    strCatZN(this->str, component, length);
    lstAdd(this->componentsInfo, &newInfo);

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
static Path *
pathAppendNonRootComponentStr(Path *const this, const String *const component)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRING, component);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);

    FUNCTION_TEST_RETURN(PATH, pathAppendNonRootComponentZN(this, strZ(component), strSize(component)));
}

/**********************************************************************************************************************************/
static Path *
pathAppendNonRootComponentZ(Path *const this, const char *const component)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRINGZ, component);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);

    FUNCTION_TEST_RETURN(PATH, pathAppendNonRootComponentZN(this, component, strlen(component)));
}
/**********************************************************************************************************************************/
static Path *
pathPrependNonRootComponentStr(Path *const this, const String *const component)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRING, component);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);
    ASSERT(!strEmpty(component));

    if (this->componentsInfo == NULL || lstEmpty(this->componentsInfo))
        pathSetRootComponentZN(this, pathRootNone, NULL, 0);



    strLstInsert(this->components, 1, component);

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
static void
pathPrependNonRootComponentZN(Path *this, const char *component, size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        /* FIXME: use FUNCTION_TEST_PARAM instead? (everywhere) */
        FUNCTION_TEST_PARAM_P(CHARDATA, component);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);
    ASSERT(length > 0);

    MEM_CONTEXT_TEMP_BEGIN();
    {
        pathPrependNonRootComponentStr(this, strNewZN(component, length));
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
static void
pathPrependNonRootComponentZ(Path *this, const char *component)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRINGZ, component);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);

    pathPrependNonRootComponentZN(this, component, strlen(component));

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
static size_t
pathParseOptionalRoot(Path *this, const char *path, size_t length)
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

        pathSetRootComponentZN(this, pathRootSlash, path, 1);
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

        pathSetRootComponentZN(this, pathRootExpression, path, rootSize);

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
pathParseNextComponent(Path *this, const char *path, size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM_P(CHARDATA, path);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    size_t consumedSize = 0;

    // Verify that the next component is valid and get its size
    while (consumedSize < length && !pathIsValidDirectorySeparatorChar(path[consumedSize]))
    {
        if (!pathIsValidComponentChar(path[consumedSize]))
            THROW(AssertError, "invalid component character found in path");

        consumedSize++;
    }

    // The component length will be zero if there is a sequence of directory separators
    if (consumedSize > 0)
        pathAppendNonRootComponentZN(this, path, consumedSize);

    // If the component ended with a directory separator, "consume" it
    if (consumedSize < length)
        consumedSize++;

    FUNCTION_TEST_RETURN(SIZE, consumedSize);
}

/**********************************************************************************************************************************/
static Path *
pathClean(Path *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    if (this->components != NULL)
    {
        unsigned int componentIdx = 1;

        while (componentIdx < strLstSize(this->components))
        {
            String *component = strLstGet(this->components, componentIdx);

            // Just remove the '.'
            if (strEq(component, DOT_STR))
            {
                strLstRemoveIdx(this->components, componentIdx);
            }
            // If we found a ".." we need to remove it and the parent component
            else if (componentIdx > 1 && strEq(component, DOTDOT_STR))
            {
                String *parentComponent = strLstGet(this->components, componentIdx - 1);

                if (!strEq(parentComponent, DOTDOT_STR))
                {
                    strLstRemoveIdx(this->components, componentIdx);
                    strLstRemoveIdx(this->components, componentIdx - 1);

                    componentIdx--;
                }
            }
            else
            {
                componentIdx++;
            }
        }

        if (this->rootType != pathRootNone)
        {
            if (strLstSize(this->components) > 1 && strEq(strLstGet(this->components, 1), DOTDOT_STR))
                THROW(AssertError, "the path cannot go back past the root");
        }
    }

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
static Path *
pathJoin(Path *const this, const Path *const basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);

    pathSetRootComponentStr(this, pathGetRootType(basePath), pathGetRoot(basePath));

    if (pathGetComponentCount(basePath) > 0)
    {
        for (unsigned int idx = pathGetComponentCount(basePath) - 1; idx > 0; idx--)
            pathPrependNonRootComponentStr(this, pathGetComponent(basePath, idx));
    }

    FUNCTION_TEST_RETURN(PATH, pathClean(this));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathNew(const String *const path)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, path);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);

    FUNCTION_TEST_RETURN(PATH, pathNewZN(strZ(path), strSize(path)));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathNewZ(const char *const path)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRINGZ, path);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);

    FUNCTION_TEST_RETURN(PATH, pathNewZN(path, strlen(path)));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathNewZN(const char *path, size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM_P(CHARDATA, path);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);
    ASSERT(length > 0);

    Path *result = pathNewInternal();

    MEM_CONTEXT_OBJ_BEGIN(result)
    {
        size_t consumedSize = pathParseOptionalRoot(result, path, length);

        do
        {
            path += consumedSize;
            length -= consumedSize;
        } while ((consumedSize = pathParseNextComponent(result, path, length)) > 0);

        pathClean(result);
    }
    MEM_CONTEXT_OBJ_END();

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathNewAbsolute(const String *const path, const Path *const basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, path);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);
    ASSERT(basePath != NULL);

    FUNCTION_TEST_RETURN(PATH, pathMakeAbsolute(pathNew(path), basePath));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathNewAbsoluteZ(const char *const path, const Path *const basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRINGZ, path);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);
    ASSERT(basePath != NULL);

    FUNCTION_TEST_RETURN(PATH, pathMakeAbsolute(pathNewZ(path), basePath));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathNewAbsoluteZN(const char *const path, const size_t length, const Path *const basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM_P(CHARDATA, path);
        FUNCTION_TEST_PARAM(SIZE, length);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);
    ASSERT(length > 0);
    ASSERT(basePath != NULL);

    FUNCTION_TEST_RETURN(PATH, pathMakeAbsolute(pathNewZN(path, length), basePath));
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

        if (this->components != NULL)
            result->components = strLstDup(this->components);
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

    FUNCTION_TEST_RETURN(BOOL, this->rootType != pathRootNone && pathGetComponentCount(this) == 1);
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

    bool result = false;

    // Both paths must be based on the same root
    if (this->rootType == basePath->rootType && strEq(pathGetRoot(this), pathGetRoot(basePath)))
    {
        if (pathIsRoot(basePath))
        {
            result = true;
        }
        else if (pathGetComponentCount(this) > 1 && pathGetComponentCount(this) >= pathGetComponentCount(basePath))
        {
            result = true;

            for (unsigned int idx = 1; idx < pathGetComponentCount(basePath); idx++)
            {
                if (!strEq(pathGetComponent(this, idx), pathGetComponent(basePath, idx)))
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

    const String *result = STR("");

    switch (this->rootType) {
        case pathRootExpression:
        case pathRootSlash:
            result = pathGetComponent(this, 0);
            break;

        case pathRootNone:
            break;
    }

    FUNCTION_TEST_RETURN(STRING, result);
}

/**********************************************************************************************************************************/
FN_EXTERN const String *
pathGetName(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    const String *result = STR("");

    unsigned int componentCount = pathGetComponentCount(this);

    if (componentCount > 1)
        result = pathGetComponent(this, componentCount - 1);

    FUNCTION_TEST_RETURN(STRING, result);
}

/**********************************************************************************************************************************/
FN_EXTERN const String *
pathGetComponent(const Path *this, unsigned int idx)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(UINT, idx);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(idx < pathGetComponentCount(this));

    FUNCTION_TEST_RETURN(STRING, strLstGet(this->components, idx));
}

/**********************************************************************************************************************************/
FN_EXTERN unsigned int
pathGetComponentCount(const Path *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(UINT, this->components == NULL ? 0 : strLstSize(this->components));
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

    if (pathGetComponentCount(result) > 1)
    {
        pathAppendNonRootComponentStr(result, DOTDOT_STR);
        pathClean(result);
    }

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN const String *
pathStr(const Path *this)
{

}

/**********************************************************************************************************************************/
//FN_EXTERN String *
//pathToString(const Path *this)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(PATH, this);
//    FUNCTION_TEST_END();
//
//    ASSERT(this != NULL);
//
//    String *result = strNew();
//
//    for (unsigned int idx = 0; idx < pathGetComponentCount(this); idx++)
//    {
//        if (idx > 0 && !strEmpty(result) && !strEndsWith(result, FSLASH_STR))
//            strCat(result, FSLASH_STR);
//
//        strCat(result, pathGetComponent(this, idx));
//    }
//
//    if (strEmpty(result))
//        result = strCat(result, DOT_STR);
//
//    FUNCTION_TEST_RETURN(STRING, result);
//}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathMakeAbsolute(Path *const this, const Path *const basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(pathIsAbsolute(basePath));

    Path *result = this;

    if (!pathIsAbsolute(this))
        result = pathJoin(this, basePath);

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathMakeRelativeTo(Path *const this, const Path *const basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(pathIsAbsolute(this));
    ASSERT(pathIsAbsolute(basePath));

    // Both paths must be based on the same root
    if (this->rootType == basePath->rootType && strEq(pathGetRoot(this), pathGetRoot(basePath)))
    {
        unsigned int baseComponentIdx = 1;
        unsigned int thisComponentIdx = 1;

        // Find the common prefix between the two paths
        while (thisComponentIdx < pathGetComponentCount(this) && baseComponentIdx < pathGetComponentCount(basePath))
        {
            if (!strEq(pathGetComponent(this, thisComponentIdx), pathGetComponent(basePath, baseComponentIdx)))
                break;

            thisComponentIdx++;
            baseComponentIdx++;
        }

        // Remove the root
        pathSetRootComponentZN(this, pathRootNone, NULL, 0);

        // Remove the common prefix
        while (thisComponentIdx > 1) {
            strLstRemoveIdx(this->components, 1);
            thisComponentIdx--;
        }

        // If the path is not relative to the base path go back the needed levels
        while (baseComponentIdx < pathGetComponentCount(basePath)) {
            strLstInsert(this->components, 1, DOTDOT_STR);
            baseComponentIdx++;
        }

        pathClean(this);
    }

    FUNCTION_TEST_RETURN(PATH, this);
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

    FUNCTION_TEST_RETURN(PATH, pathJoin(pathDup(this), basePath));
}

/**********************************************************************************************************************************/
FN_EXTERN void
pathToLog(const Path *this, StringStatic *debugLog)
{
    strStcCat(debugLog, "{root: ");
    strStcResultSizeInc(debugLog, FUNCTION_LOG_ENUM_FORMAT(this->rootType, strStcRemains(debugLog), strStcRemainsSize(debugLog)));
    strStcCat(debugLog, ", components: ");
    strStcResultSizeInc(debugLog, FUNCTION_LOG_STRING_LIST_FORMAT(this->components, strStcRemains(debugLog), strStcRemainsSize(debugLog)));
    strStcCatChr(debugLog, '}');
}
