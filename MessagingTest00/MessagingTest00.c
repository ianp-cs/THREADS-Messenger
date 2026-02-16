#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

/*********************************************************************************
*
* MessagingTest00 - Smoke Test
*
* Verifies that the Messaging system starts and exits cleanly. No mailbox
* operations are performed. This test validates that SchedulerEntryPoint
* correctly initializes the messaging layer and spawns the MessagingEntryPoint
* process.
*
* Expected: Process starts, prints a message, and exits with code 0.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
    char* testName = GetTestName(__FILE__);

    /* Just output a message and exit. */
    console_output(FALSE, "\n%s: started\n", testName);

    k_exit(0);

    return 0;
}
