/***********************************************************************************************************************************
Common Handler for Control Commands
***********************************************************************************************************************************/
#include "build.auto.h"

#include "command/control/common.h"
#include "common/debug.h"
#include "config/config.h"
#include "storage/helper.h"

/**********************************************************************************************************************************/
FN_EXTERN Path *
lockStopFilePath(const String *const stanza)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, stanza);
    FUNCTION_TEST_END();

    Path *const result = pathDup(cfgOptionPath(cfgOptLockPath));

    if (stanza != NULL)
        pathAppendComponentFmt(result, "%s" STOP_FILE_EXT, strZ(stanza));
    else
        pathAppendComponentZ(result, "all" STOP_FILE_EXT);

    FUNCTION_TEST_RETURN(PATH, result);
}

/**********************************************************************************************************************************/
FN_EXTERN void
lockStopTest(void)
{
    FUNCTION_LOG_VOID(logLevelDebug);

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Check the current stanza (if any)
        if (cfgOptionTest(cfgOptStanza))
        {
            if (storageExistsP(storageLocal(), lockStopFilePath(cfgOptionStr(cfgOptStanza))))
                THROW_FMT(StopError, "stop file exists for stanza %s", strZ(cfgOptionDisplay(cfgOptStanza)));
        }

        // Check all stanzas
        if (storageExistsP(storageLocal(), lockStopFilePath(NULL)))
            THROW(StopError, "stop file exists for all stanzas");
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}
