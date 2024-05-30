#include "build.auto.h"

#include "common/path.h"
#include "common/type/stringList.h"

/**********************************************************************************************************************************/
FN_EXTERN bool
pathIsAbsolute(const String *const path)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, path);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);

    FUNCTION_TEST_RETURN(BOOL, strSize(path) >= 1 && strZ(path)[0] == '/');
}

/**********************************************************************************************************************************/
FN_EXTERN bool
pathIsRelative(const String *const path)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, path);
    FUNCTION_TEST_END();

    ASSERT(path != NULL);

    FUNCTION_TEST_RETURN(BOOL, !pathIsAbsolute(path));
}

/**********************************************************************************************************************************/
FN_EXTERN bool
pathIsRelativeTo(const String *const basePath, const String *const path)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, basePath);
        FUNCTION_TEST_PARAM(STRING, path);
    FUNCTION_TEST_END();

    ASSERT(basePath != NULL && pathIsAbsolute(basePath));
    ASSERT(path != NULL && pathIsAbsolute(path));

    bool result =
        strEqZ(basePath, "/") ||
        (strBeginsWith(path, basePath) &&
        (strSize(basePath) == strSize(path) || *(strZ(path) + strSize(basePath)) == '/'));

    FUNCTION_TEST_RETURN(BOOL, result);
}

/**********************************************************************************************************************************/
FN_EXTERN String *
pathMakeAbsolute(const String *const basePath, const String *const path)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, basePath);
        FUNCTION_TEST_PARAM(STRING, path);
    FUNCTION_TEST_END();

    ASSERT(basePath != NULL && pathIsAbsolute(basePath));
    ASSERT(path != NULL);

    String *result = NULL;

    if (pathIsRelative(path))
    {
        if (strEqZ(basePath, "/"))
            result = strNewFmt("/%s", strZ(path));
        else
            result = strNewFmt("%s/%s", strZ(basePath), strZ(path));
    }
    else
        result = strDup(path);

    FUNCTION_TEST_RETURN(STRING, result);
}

/**********************************************************************************************************************************/
FN_EXTERN String *
pathMakeRelativeTo(const String *basePath, const String *path)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, basePath);
        FUNCTION_TEST_PARAM(STRING, path);
    FUNCTION_TEST_END();

    ASSERT(basePath != NULL && pathIsAbsolute(basePath));
    ASSERT(path != NULL && pathIsAbsolute(path));

    String *result = strNew();

    MEM_CONTEXT_TEMP_BEGIN();
    {
        StringList *baseComponents = strLstNewSplitZ(basePath, "/");
        StringList *pathComponents = strLstNewSplitZ(path, "/");
        unsigned int baseComponentsIdx = 1;
        unsigned int baseComponentsSize = strLstSize(baseComponents);
        unsigned int pathComponentsIdx = 1;
        unsigned int pathComponentsSize = strLstSize(pathComponents);

        // Find the common prefix between the two paths
        while (baseComponentsIdx < baseComponentsSize && pathComponentsIdx < pathComponentsSize)
        {
            if (!strEq(strLstGet(baseComponents, baseComponentsIdx), strLstGet(pathComponents, pathComponentsIdx)))
                break;

            baseComponentsIdx++;
            pathComponentsIdx++;
        }

        // If the path is not relative to the base path go back some levels
        for (unsigned int idx = baseComponentsIdx; idx < baseComponentsSize; idx++)
            strCatZ(result, "../");

        // Append the relative part
        for (unsigned int idx = pathComponentsIdx; idx < pathComponentsSize; idx++)
        {
            strCat(result, strLstGet(pathComponents, idx));

            if ((idx + 1) != pathComponentsSize)
                strCatZN(result, "/", 1);
        }

        // If both paths are equal, return the base path
        if (strEmpty(result))
            strCat(result, basePath);
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_TEST_RETURN(STRING, result);
}
