#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"



/*********************************************************************************
*
* MessagingTest35 - mailbox_create Boundary Validation
*
* Tests three invalid parameter combinations for mailbox_create:
*
*   1. slot_size = MAX_MESSAGE + 1 (exceeds maximum allowed message size)
*   2. slot_size = -1 (negative slot size)
*   3. slots = -1 (negative slot count)
*
* Each call should fail and return -1 since the parameters are out of the
* valid range.
*
* Tests input validation in mailbox_create for boundary and invalid values.
*
* Expected: All three mailbox_create calls return -1 (PASSED).
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
    char* testName = GetTestName(__FILE__);
    char childNames[MAXPROC][256];
    int childId = 0;
    int mailboxId;

    memset(childNames, 0, sizeof(childNames));

    console_output(FALSE, "\n%s: started\n", testName);

    mailboxId = mailbox_create(10, MAX_MESSAGE + 1);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);
    if (mailboxId == -1)
    {
        console_output(FALSE, "%s: mailbox_create border case test 1: PASSED\n", testName);
    }
    else
    {
        console_output(FALSE, "%s: mailbox_create border case test 1 returned %d: FAILED\n", testName, mailboxId);

    }

    mailboxId = mailbox_create(10, -1);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);
    if (mailboxId == -1)
    {
        console_output(FALSE, "%s: mailbox_create border case test 2: PASSED\n", testName);
    }
    else
    {
        console_output(FALSE, "%s: mailbox_create border case test 2 returned %d: FAILED\n", testName, mailboxId);

    }

    mailboxId = mailbox_create(-1, 10);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);
    if (mailboxId == -1)
    {
        console_output(FALSE, "%s: mailbox_create border case test 3: PASSED\n", testName);
    }
    else
    {
        console_output(FALSE, "%s: mailbox_create border case test 3 returned %d: FAILED\n", testName, mailboxId);

    }

    k_exit(0);

    return 0;
} /* MessagingEntryPoint */
