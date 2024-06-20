#include "build.auto.h"

#include <stdarg.h>
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
    StringList *components;
    String *cachedString;
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
static bool
pathComponentIsAlwaysADirectory(const String *const component)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, component);
    FUNCTION_TEST_END();

    FUNCTION_TEST_RETURN(BOOL, strEq(component, DOTDOT_STR) || strEq(component, DOT_STR));
}

/**********************************************************************************************************************************/
static FN_PRINTF(3, 0) char *
pathInternalFormatHelper(char *const buffer, size_t *const size, const char *const format, va_list argumentList)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM_P(CHARDATA, buffer);
        FUNCTION_TEST_PARAM_P(SIZE, size);
        FUNCTION_TEST_PARAM(STRINGZ, format);
    FUNCTION_TEST_END();

    ASSERT(buffer != NULL);
    ASSERT(size != NULL);
    ASSERT(format != NULL);

    char *result = buffer;

    // First try to format into the buffer
    int formatResult = vsnprintf(result, *size, format, argumentList);

    // If the buffer is insufficient allocate more memory and try again
    if (formatResult > 0 && ((size_t) formatResult) >= *size)
    {
        result = memNew((size_t) formatResult + 1);

        formatResult = vsnprintf(result, (size_t) formatResult + 1, format, argumentList);
    }

    if (formatResult < 0)
        THROW_FMT(AssertError, "vsnprintf returned %d", formatResult);

    *size = (size_t) formatResult;

    FUNCTION_TEST_RETURN(STRINGZ, result);
}

/**********************************************************************************************************************************/
static Path *
pathInternalNew(void)
{
    FUNCTION_TEST_VOID();

    OBJ_NEW_BEGIN(Path, .childQty = MEM_CONTEXT_QTY_MAX)
    {
        *this = (Path)
        {
            .rootType = pathRootNone,
            .components = strLstNew(),
            .cachedString = NULL,
        };

        strLstAdd(this->components, STR(""));
    }
    OBJ_NEW_END();

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
static Path *
pathInvalidateCache(Path *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    if (this->cachedString != NULL)
    {
        strFree(this->cachedString);
        this->cachedString = NULL;
    }

    FUNCTION_TEST_RETURN(PATH, this);
}

/**********************************************************************************************************************************/
static size_t
pathGetNonRootComponentSizeZN(const char *const component, const size_t length, bool stopAtDirectorySeparator)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM_P(CHARDATA, path);
        FUNCTION_TEST_PARAM(SIZE, length);
        FUNCTION_TEST_PARAM(BOOL, stopAtDirectorySeparator);
    FUNCTION_TEST_END();

    size_t componentSize = 0;

    // Verify that the next component is valid and get its size
    while (componentSize < length)
    {
        if (stopAtDirectorySeparator && pathIsValidDirectorySeparatorChar(component[componentSize]))
            break;

        if (!pathIsValidComponentChar(component[componentSize]))
            THROW(AssertError, "invalid component character found");

        componentSize++;
    }

    FUNCTION_TEST_RETURN(SIZE, componentSize);
}

/**********************************************************************************************************************************/
static Path *
pathSetRootComponentZN(Path *const this, const PathRootType rootType, const char *const root, const size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(ENUM, rootType);
        FUNCTION_TEST_PARAM_P(CHARDATA, root);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(root != NULL);
    ASSERT((rootType == pathRootNone && length == 0) || (rootType != pathRootNone && length > 0));

    strLstRemoveIdx(this->components, 0);
    strLstInsert(this->components, 0, STR_SIZE(root, length));

    this->rootType = rootType;

    FUNCTION_TEST_RETURN(PATH, pathInvalidateCache(this));
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
        FUNCTION_TEST_PARAM_P(CHARDATA, component);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);
    ASSERT(length > 0);

    strLstAddZSub(this->components, component, length);

    FUNCTION_TEST_RETURN(PATH, pathInvalidateCache(this));
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
        FUNCTION_TEST_PARAM_P(CHARDATA, component);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);
    ASSERT(length > 0);

    strLstInsert(this->components, 1, STR_SIZE(component, length));

    FUNCTION_TEST_RETURN(PATH, pathInvalidateCache(this));
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
        pathSetRootComponentZN(this, pathRootNone, "", 0);
    else
        strLstRemoveIdx(this->components, index);

    FUNCTION_TEST_RETURN(PATH, pathInvalidateCache(this));
}

/**********************************************************************************************************************************/
static const String *
pathInternalGetName(const Path *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    const String *result = NULL;
    const unsigned int componentCount = strLstSize(this->components);

    if (componentCount > 1)
    {
        result = strLstGet(this->components, componentCount - 1);

        if (pathComponentIsAlwaysADirectory(result))
            result = NULL;
    }

    FUNCTION_TEST_RETURN_CONST(STRING, result);
}

/**********************************************************************************************************************************/
static Path *
pathInternalSetNameZN(Path *const this, const char *const name, const size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM_P(CHARDATA, name);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(name != NULL);
    ASSERT(length > 0);

    if (pathComponentIsAlwaysADirectory(STR_SIZE(name, length)) ||
        pathGetNonRootComponentSizeZN(name, length, false) != length)
    {
        THROW_FMT(AssertError, "'%.*s' is not valid name", (int) length, name);
    }

    if (pathInternalGetName(this) != NULL)
    {
        const unsigned int nameIndex = strLstSize(this->components) - 1;

        strLstRemoveIdx(this->components, nameIndex);
        strLstInsert(this->components, nameIndex, STR_SIZE(name, length));
    }
    else
        pathAppendNonRootComponentZN(this, name, length);

    FUNCTION_TEST_RETURN(PATH, pathInvalidateCache(this));
}

/**********************************************************************************************************************************/
static size_t
pathParseOptionalRoot(Path *const this, const char *const path, const size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
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
pathParseNextNonRootComponent(Path *const this, const char *const path, const size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM_P(CHARDATA, path);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    size_t consumedSize = pathGetNonRootComponentSizeZN(path, length, true);

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
pathSetBase(Path *const this, const Path *const basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);

    pathSetRootComponentStr(this, pathGetRootType(basePath), pathGetRoot(basePath));

    if (pathGetComponentCount(basePath) > 1)
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
        /* FIXME: do not allow // to be present on the path (we are currently ignoring it) */

        size_t consumedSize = pathParseOptionalRoot(result, path, length);

        do
        {
            path += consumedSize;
            length -= consumedSize;
        } while ((consumedSize = pathParseNextNonRootComponent(result, path, length)) > 0);

        pathClean(result);
    }
    MEM_CONTEXT_OBJ_END();

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN FN_PRINTF(1, 2) Path *
pathNewFmt(const char *format, ...)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRINGZ, format);
    FUNCTION_TEST_END();

    ASSERT(format != NULL);

    Path *result = NULL;

    MEM_CONTEXT_TEMP_BEGIN();
    {
        char pathBuffer[64];
        size_t pathSize = sizeof(pathBuffer);
        va_list argumentList;

        va_start(argumentList, format);
        char *path = pathInternalFormatHelper(pathBuffer, &pathSize, format, argumentList);
        va_end(argumentList);

        MEM_CONTEXT_PRIOR_BEGIN();
        {
            result = pathNewZN(path, pathSize);
        }
        MEM_CONTEXT_PRIOR_END();
    }
    MEM_CONTEXT_TEMP_END();

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

    FUNCTION_TEST_RETURN_CONST(STRING, pathGetComponent(this, 0));
}

/**********************************************************************************************************************************/
FN_EXTERN bool
pathHasName(const Path *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(BOOL, pathInternalGetName(this) != NULL);
}

/**********************************************************************************************************************************/
FN_EXTERN const String *
pathGetName(const Path *const this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    const String *result = pathInternalGetName(this);

    if (result == NULL)
        result = STR("");

    FUNCTION_TEST_RETURN_CONST(STRING, result);
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathSetName(Path *const this, const String *const name)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRING, name);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(name != NULL);

    FUNCTION_TEST_RETURN(PATH, pathInternalSetNameZN(this, strZ(name), strSize(name)));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathSetNameZ(Path *const this, const char *const name)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRINGZ, name);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(name != NULL);

    FUNCTION_TEST_RETURN(PATH, pathInternalSetNameZN(this, name, strlen(name)));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathSetNameZN(Path *const this, const char *const name, const size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM_P(CHARDATA, name);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(name != NULL);
    ASSERT(length > 0);

    FUNCTION_TEST_RETURN(PATH, pathInternalSetNameZN(this, name, length));
}

/**********************************************************************************************************************************/
FN_EXTERN FN_PRINTF(2, 3) Path *
pathSetNameFmt(Path *const this, const char *const format, ...)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRINGZ, format);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(format != NULL);

    MEM_CONTEXT_TEMP_BEGIN();
    {
        char nameBuffer[64];
        size_t nameSize = sizeof(nameBuffer);
        va_list argumentList;

        va_start(argumentList, format);
        char *name = pathInternalFormatHelper(nameBuffer, &nameSize, format, argumentList);
        va_end(argumentList);

        pathInternalSetNameZN(this, name, nameSize);
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_TEST_RETURN(PATH, this);
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

    FUNCTION_TEST_RETURN_CONST(STRING, strLstGet(this->components, index));
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
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathAppendComponent(Path *const this, const String *const component)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRING, component);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);

    FUNCTION_TEST_RETURN(PATH, pathAppendComponentZN(this, strZ(component), strSize(component)));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathAppendComponentZ(Path *const this, const char *const component)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRINGZ, component);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);

    FUNCTION_TEST_RETURN(PATH, pathAppendComponentZN(this, component, strlen(component)));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathAppendComponentZN(Path *const this, const char *const component, const size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM_P(CHARDATA, component);
        FUNCTION_TEST_PARAM(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(component != NULL);
    ASSERT(length > 0);

    if (pathGetNonRootComponentSizeZN(component, length, false) != length)
        THROW_FMT(AssertError, "'%.*s' is not a valid path component", (int) length, component);

    FUNCTION_TEST_RETURN(PATH, pathClean(pathAppendNonRootComponentZN(this, component, length)));
}

/**********************************************************************************************************************************/
FN_EXTERN FN_PRINTF(2, 3) Path *
pathAppendComponentFmt(Path *const this, const char *const format, ...)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRINGZ, format);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(format != NULL);

    MEM_CONTEXT_TEMP_BEGIN();
    {
        char nameBuffer[64];
        size_t nameSize = sizeof(nameBuffer);
        va_list argumentList;

        va_start(argumentList, format);
        char *name = pathInternalFormatHelper(nameBuffer, &nameSize, format, argumentList);
        va_end(argumentList);

        pathAppendComponentZN(this, name, nameSize);
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_TEST_RETURN(PATH, this);
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
FN_EXTERN bool
pathEq(const Path *const this, const Path *const compare)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, compare);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(compare != NULL);

    FUNCTION_TEST_RETURN(BOOL, pathCmp(this, compare) == 0);
}

/**********************************************************************************************************************************/
FN_EXTERN int
pathCmp(const Path *const this, const Path *const compare)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, compare);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(compare != NULL);

    int result;

    if (this->rootType < compare->rootType)
        result = -1;
    else if (this->rootType > compare->rootType)
        result = 1;
    else
    {
        unsigned int thisComponentCount = pathGetComponentCount(this);
        unsigned int compareComponentCount = pathGetComponentCount(compare);

        if (thisComponentCount < compareComponentCount)
            result = -1;
        else if (thisComponentCount > compareComponentCount)
            result = 1;
        else
        {
            result = 0;

            for (unsigned int idx = 0; result == 0 && idx < thisComponentCount; idx++)
                result = strCmp(pathGetComponent(this, idx), pathGetComponent(compare, idx));
        }
    }

    FUNCTION_TEST_RETURN(INT, result);
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

    FUNCTION_TEST_RETURN_CONST(STRING, nonConstThis->cachedString);
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathJoin(Path *const this, const Path *const basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(pathIsRelative(this));

    FUNCTION_TEST_RETURN(PATH, pathSetBase(this, basePath));
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
        result = pathSetBase(this, basePath);

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
        pathSetRootComponentZN(this, pathRootNone, "", 0);

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
FN_EXTERN Path *
pathResolveExpression(const Path *const this, const Path *const basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(PATH, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(this->rootType == pathRootExpression);

    FUNCTION_TEST_RETURN(PATH, pathSetBase(pathDup(this), basePath));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathResolveExpressionStr(const Path *const this, const String *const basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRING, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(this->rootType == pathRootExpression);

    FUNCTION_TEST_RETURN(PATH, pathResolveExpressionZN(this, strZ(basePath), strSize(basePath)));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathResolveExpressionZ(const Path *const this, const char *const basePath)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRINGZ, basePath);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(this->rootType == pathRootExpression);

    FUNCTION_TEST_RETURN(PATH, pathResolveExpressionZN(this, basePath, strlen(basePath)));
}

/**********************************************************************************************************************************/
FN_EXTERN Path *
pathResolveExpressionZN(const Path *const this, const char *const basePath, const size_t length)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM_P(CHARDATA, basePath);
        FUNCTION_TEST_PARAM_P(SIZE, length);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(basePath != NULL);
    ASSERT(length > 0);
    ASSERT(this->rootType == pathRootExpression);

    Path *result = NULL;

    MEM_CONTEXT_TEMP_BEGIN();
    {
        Path *base = pathNewZN(basePath, length);

        MEM_CONTEXT_PRIOR_BEGIN();
        {
            result = pathResolveExpression(this, base);
        }
        MEM_CONTEXT_PRIOR_END();
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN FN_PRINTF(2, 3) Path *
pathResolveExpressionFmt(const Path *const this, const char *const format, ...)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(PATH, this);
        FUNCTION_TEST_PARAM(STRINGZ, format);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);
    ASSERT(format != NULL);

    Path *result = NULL;

    MEM_CONTEXT_TEMP_BEGIN();
    {
        char pathBuffer[128];
        size_t pathSize = sizeof(pathBuffer);
        va_list argumentList;

        va_start(argumentList, format);
        char *path = pathInternalFormatHelper(pathBuffer, &pathSize, format, argumentList);
        va_end(argumentList);

        MEM_CONTEXT_PRIOR_BEGIN();
        {
            result = pathResolveExpressionZN(this, path, pathSize);
        }
        MEM_CONTEXT_PRIOR_END();
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_TEST_RETURN(PATH, result);
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
