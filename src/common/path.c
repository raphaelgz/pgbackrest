#include "build.auto.h"

#include <string.h>

#include "common/log.h"
#include "common/path.h"
//#include "common/type/list.h"
#include "common/type/stringList.h"

/***********************************************************************************************************************************
Object types
***********************************************************************************************************************************/
//typedef struct PathComponentInfo
//{
//    size_t offset;
//    size_t length;
//} PathComponentInfo;

struct Path
{
    PathRootType rootType;
    StringList *components;
    String *cachedString;
//    String *str;
//    List *componentsInfo;
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
pathInternalNew(void)
{
    FUNCTION_TEST_VOID();

    OBJ_NEW_BEGIN(Path)
    {
        *this = (Path)
        {
            .rootType = pathRootNone,
            .components = strLstNew(),
            .cachedString = NULL,
//            .str = NULL,
//            .componentsInfo = NULL,
        };

        strLstAdd(this->components, STR(""));
    }
    OBJ_NEW_END();

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
//static Path *
//pathInternalInsertComponentZN(Path *const this, const unsigned int componentIndex, const char *const component, const size_t length)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(PATH, this);
//        FUNCTION_TEST_PARAM(UINT, componentIndex);
//        FUNCTION_TEST_PARAM(CHARDATA, component);
//        FUNCTION_TEST_PARAM(SIZE, length);
//    FUNCTION_TEST_END();
//
//    ASSERT(this != NULL);
//    ASSERT(component != NULL);
//    ASSERT(length > 0);
//
//    MEM_CONTEXT_OBJ_BEGIN(this);
//    {
//        if (this->componentsInfo == NULL)
//            this->componentsInfo = lstNewP(sizeof(PathComponentInfo), .sortOrder = sortOrderNone);
//
//        if (this->str == NULL)
//            this->str = strNew();
//
//        bool addSeparatorAfter = false;
//        bool addSeparatorBefore = false;
//        PathComponentInfo *componentInfo = NULL;
//
//        // Using a block only to make harder to use the nextComponentInfo and previousComponentInfo variables after the call to lstInsert()
//        {
//            PathComponentInfo *nextComponentInfo = NULL;
//            PathComponentInfo *previousComponentInfo = NULL;
//
//            if (componentIndex < lstSize(this->componentsInfo))
//                nextComponentInfo = (PathComponentInfo *) lstGet(this->componentsInfo, componentIndex);
//
//            if (componentIndex > 0 && componentIndex <= lstSize(this->componentsInfo))
//                previousComponentInfo = (PathComponentInfo *) lstGet(this->componentsInfo, componentIndex - 1);
//
//            // If the new component is not the first, calculate the offset based on the previous component
//            if (previousComponentInfo != NULL)
//            {
//                PathComponentInfo newComponentInfo =
//                {
//                    .offset = previousComponentInfo->offset + previousComponentInfo->length,
//                    .length = length,
//                };
//
//                // Ensure that the new component is separated from the previous one with a directory separator
//                if (!pathIsValidDirectorySeparatorChar(strZ(this->str)[newComponentInfo.offset - 1]))
//                    addSeparatorBefore = true;
//                // Ensure that the new component is separated from the next one with a directory separator
//                else if (nextComponentInfo != NULL)
//                    addSeparatorAfter = true;
//
//                componentInfo = (PathComponentInfo *) lstInsert(this->componentsInfo, componentIndex, &newComponentInfo);
//            }
//            else
//            {
//                // Ensure that the new component is separated from the next one with a directory separator
//                if (nextComponentInfo != NULL && !pathIsValidDirectorySeparatorChar(component[length - 1]))
//                {
//                    if (nextComponentInfo->offset == 0 || !pathIsValidDirectorySeparatorChar(strZ(this->str)[nextComponentInfo->offset - 1]))
//                        addSeparatorAfter = true;
//                }
//
//                PathComponentInfo newComponentInfo =
//                {
//                    .offset = 0,
//                    .length = length,
//                };
//
//                componentInfo = (PathComponentInfo *) lstInsert(this->componentsInfo, componentIndex, &newComponentInfo);
//            }
//        }
//
//        // Update the path string
//        if (addSeparatorAfter)
//            strInsert(this->str, componentInfo->offset, FSLASH_STR);
//
//        strInsertZN(this->str, componentInfo->offset, component, length);
//
//        if (addSeparatorAfter)
//        {
//            strInsert(this->str, componentInfo->offset, FSLASH_STR);
//            componentInfo->offset++;
//        }
//
//        // Update the offset of all components that come after the new component
//        for (unsigned int idx = componentIndex + 1; idx < lstSize(this->componentsInfo); idx++)
//        {
//            PathComponentInfo *info = (PathComponentInfo *) lstGet(this->componentsInfo, idx);
//
//            info->offset += componentInfo->length;
//
//            if (addSeparatorAfter)
//                info->offset++;
//
//            if (addSeparatorBefore)
//                info->offset++;
//        }
//    }
//    MEM_CONTEXT_OBJ_END();
//
//    FUNCTION_TEST_RETURN(PATH, this);
//}

/**********************************************************************************************************************************/
//static Path *
//pathInternalRemoveComponent(Path *const this, const unsigned int componentIndex)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(PATH, this);
//        FUNCTION_TEST_PARAM(UINT, componentIndex);
//    FUNCTION_TEST_END();
//
//    ASSERT(this != NULL);
//    ASSERT(this->componentsInfo != NULL && componentIndex < lstSize(this->componentsInfo));
//
//    PathComponentInfo *componentInfo = (PathComponentInfo *) lstGet(this->componentsInfo, componentIndex);
//    const size_t componentEndStringIndex = componentInfo->offset + componentInfo->length;
//
//    // Remove the directory separator if it is not part of the component
//    if (!pathIsValidDirectorySeparatorChar(strZ(this->str)[componentEndStringIndex - 1]))
//    {
//        if (componentEndStringIndex < strSize(this->str) && pathIsValidDirectorySeparatorChar(strZ(this->str)[componentEndStringIndex]))
//            componentInfo->length++; // Increase the length in order to "magically" remove the directory separator without any additional checks
//    }
//
//    // Update the offset of all components that come after the removed component
//    for (unsigned int idx = componentIndex + 1; idx < lstSize(this->componentsInfo); idx++)
//    {
//        PathComponentInfo *info = (PathComponentInfo *) lstGet(this->componentsInfo, idx);
//
//        info->offset -= componentInfo->length;
//    }
//
//    strRemove(this->str, componentInfo->offset, componentInfo->length);
//    lstRemoveIdx(this->componentsInfo, componentIndex);
//
//    FUNCTION_TEST_RETURN(PATH, this);
//}

/**********************************************************************************************************************************/
//static Path *
//pathInternalReplaceComponentZN(Path *const this, const unsigned int componentIndex, const char *const component, const size_t length)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(PATH, this);
//        FUNCTION_TEST_PARAM(UINT, componentIndex);
//        FUNCTION_TEST_PARAM(CHARDATA, component);
//        FUNCTION_TEST_PARAM(SIZE, length);
//    FUNCTION_TEST_END();
//
//    ASSERT(this != NULL);
//    ASSERT(this->componentsInfo != NULL && componentIndex < lstSize(this->componentsInfo));
//    ASSERT(component != NULL);
//    ASSERT(length > 0);
//
//    pathInternalRemoveComponent(this, componentIndex);
//
//    FUNCTION_TEST_RETURN(PATH, pathInternalInsertComponentZN(this, componentIndex, component, length));
//}

/**********************************************************************************************************************************/
static Path *
pathSetRootComponentZN(Path *const this, const PathRootType rootType, const char *const root, const size_t length)
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

    String *const component = strLstGet(this->components, 0);

    if (rootType == pathRootNone)
        strTrunc(component);
    else
        strCatZN(strTrunc(component), root, length);

//    if (rootType == pathRootNone)
//    {
//        if (this->rootType != rootType)
//            pathInternalRemoveComponent(this, 0);
//    }
//    else
//    {
//        if (this->rootType == pathRootNone)
//            pathInternalInsertComponentZN(this, 0, root, length);
//        else
//            pathInternalReplaceComponentZN(this, 0, root, length);
//    }

    this->rootType = rootType;

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

    strLstAddZSub(this->components, component, length);

    FUNCTION_TEST_RETURN(PATH, this);
//    FUNCTION_TEST_RETURN(
//        PATH,
//        pathInternalInsertComponentZN(this,
//                                      this->componentsInfo == NULL ? 0 : lstSize(this->componentsInfo),
//                                      component,
//                                      length));
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
pathPrependNonRootComponentZN(Path *const this, const char *const component, const size_t length)
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

    strLstInsert(this->components, 1, STR_SIZE(component, length));

    FUNCTION_TEST_RETURN(PATH, this);

//    unsigned int componentIndex = 1;
//
//    if (this->rootType == pathRootNone)
//        componentIndex = 0;
//
//    FUNCTION_TEST_RETURN(PATH, pathInternalInsertComponentZN(this, componentIndex, component, length));
}

/**********************************************************************************************************************************/
static Path *
pathPrependNonRootComponentZ(Path *const this, const char *const component)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRINGZ, component);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);

    FUNCTION_TEST_RETURN(PATH, pathPrependNonRootComponentZN(this, component, strlen(component)));
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

    FUNCTION_TEST_RETURN(PATH, pathPrependNonRootComponentZN(this, strZ(component), strSize(component)));
}

/**********************************************************************************************************************************/
static Path *
pathRemoveComponent(Path *const this, const unsigned int index)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(UINT, index);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(index < strLstSize(this->components));

    if (index == 0)
        pathSetRootComponentZN(this, pathRootNone, NULL, 0);
    else
        strLstRemoveIdx(this->components, index);

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
static size_t
pathParseOptionalRoot(Path *const this, const char *const path, const size_t length)
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
pathParseNextComponent(Path *const this, const char *const path, const size_t length)
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

    unsigned int componentIdx = 1;

    while (componentIdx < pathGetComponentCount(this))
    {
        const String *const component = pathGetComponent(this, componentIdx);

        // Just remove the '.'
        if (strEq(component, DOT_STR))
        {
            pathRemoveComponent(this, componentIdx);
        }
        // If we found a ".." we need to remove it and the parent component
        else if (componentIdx > 1 && strEq(component, DOTDOT_STR))
        {
            const String *const parentComponent = pathGetComponent(this, componentIdx - 1);

            if (!strEq(parentComponent, DOTDOT_STR))
            {
                pathRemoveComponent(this, componentIdx);
                pathRemoveComponent(this, componentIdx - 1);

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
        if (pathGetComponentCount(this) > 1 && strEq(pathGetComponent(this, 1), DOTDOT_STR))
            THROW(AssertError, "the path cannot go back past the root");
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

    Path *result = pathInternalNew();

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

    Path *result = pathInternalNew();

    MEM_CONTEXT_OBJ_BEGIN(result)
    {
        result->rootType = this->rootType;
        result->components = strLstDup(this->components);

        if (this->cachedString != NULL)
            result->cachedString = strDup(this->cachedString);

//        if (this->componentsInfo != NULL)
//        {
//            result->componentsInfo = lstNewP(sizeof(PathComponentInfo), .sortOrder = sortOrderNone);
//
//            for (unsigned int idx = 0; idx < lstSize(this->componentsInfo); idx++)
//                lstAdd(result->componentsInfo, lstGet(this->componentsInfo, idx));
//        }
//
//        if (this->str != NULL)
//            result->str = strDup(this->str);
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
pathGetRoot(const Path *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(STRING, pathGetComponent(this, 0));
}

/**********************************************************************************************************************************/
FN_EXTERN const String *
pathGetName(const Path *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    const String *result = STR("");
    const unsigned int componentCount = pathGetComponentCount(this);

    if (componentCount > 1)
        result = pathGetComponent(this, componentCount - 1);

    FUNCTION_TEST_RETURN(STRING, result);
}

/**********************************************************************************************************************************/
FN_EXTERN const String *
pathGetComponent(const Path *const this, const unsigned int index)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(UINT, index);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(index < pathGetComponentCount(this));

    FUNCTION_TEST_RETURN(STRING, strLstGet(this->components, index));

//    const String *result = STR("");
//    const PathComponentInfo *componentInfo = NULL;
//
//    if (this->rootType == pathRootNone)
//    {
//        if (index > 0)
//            componentInfo = (PathComponentInfo *) lstGet(this->componentsInfo, index - 1);
//    }
//    else
//        componentInfo = (PathComponentInfo *) lstGet(this->componentsInfo, index);
//
//    if (componentInfo != NULL)
//        result = STR_SIZE(strZ(this->str) + componentInfo->offset, componentInfo->length);
//
//    FUNCTION_TEST_RETURN(STRING, result);
}

/**********************************************************************************************************************************/
FN_EXTERN unsigned int
pathGetComponentCount(const Path *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(UINT, strLstSize(this->components));

//    unsigned int result = this->componentsInfo == NULL ? 0 : lstSize(this->componentsInfo);
//
//    if (this->rootType == pathRootNone)
//        result++;
//
//    FUNCTION_TEST_RETURN(UINT, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathGetParent(const Path *const this)
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
pathStr(const Path *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    Path *const nonConstThis = (Path *) this;

    if (nonConstThis->cachedString == NULL)
    {
        MEM_CONTEXT_OBJ_BEGIN(nonConstThis);
        {
            nonConstThis->cachedString = strNew();

            for (unsigned int index = 0; index < strLstSize(nonConstThis->components); index++)
            {
                if (!strEmpty(nonConstThis->cachedString) && !strEndsWith(nonConstThis->cachedString, FSLASH_STR))
                    strCat(nonConstThis->cachedString, FSLASH_STR);

                strCat(nonConstThis->cachedString, strLstGet(nonConstThis->components, index));
            }
        }
        MEM_CONTEXT_OBJ_END();
    }

    FUNCTION_TEST_RETURN(STRING, nonConstThis->cachedString);

//    const String *result = STR(".");
//
//    if (this->rootType != pathRootNone || pathGetComponentCount(this) > 1)
//        result = this->str;
//
//    FUNCTION_TEST_RETURN(STRING, result);
}

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
            pathRemoveComponent(this, 1);
            thisComponentIdx--;
        }

        // If the path is not relative to the base path go back the needed levels
        while (baseComponentIdx < pathGetComponentCount(basePath)) {
            pathPrependNonRootComponentStr(this, DOTDOT_STR);
            baseComponentIdx++;
        }

        pathClean(this);
    }

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
//FN_EXTERN Path *
//pathRemoveComponent(Path *const this, const unsigned int index)
//{
//    FUNCTION_TEST_BEGIN();
//        FUNCTION_TEST_PARAM(PATH, this);
//        FUNCTION_TEST_PARAM(UINT, index);
//    FUNCTION_TEST_END();
//
//    ASSERT(this != NULL);
//    ASSERT(index < pathGetComponentCount(this));
//
//    if (index == 0)
//        pathSetRootComponentZN(this, pathRootNone, NULL, 0);
//    else if (this->rootType == pathRootNone)
//        pathInternalRemoveComponent(this, index - 1);
//    else
//        pathInternalRemoveComponent(this, index);
//
//    FUNCTION_TEST_RETURN(PATH, this);
//}

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
    strStcCat(debugLog, "{rootType: ");
    strStcResultSizeInc(debugLog, FUNCTION_LOG_ENUM_FORMAT(this->rootType, strStcRemains(debugLog), strStcRemainsSize(debugLog)));
    strStcCat(debugLog, ", components: ");
    strStcResultSizeInc(debugLog, FUNCTION_LOG_STRING_LIST_FORMAT(this->components, strStcRemains(debugLog), strStcRemainsSize(debugLog)));
    strStcCat(debugLog, ", cachedString: ");
    strStcResultSizeInc(debugLog, FUNCTION_LOG_STRING_FORMAT(this->cachedString, strStcRemains(debugLog), strStcRemainsSize(debugLog)));
    strStcCatChr(debugLog, '}');
}
