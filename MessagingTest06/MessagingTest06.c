#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

/*********************************************************************************
*
* MessagingTest06 - Blocking Send on Full Mailbox
*
* Creates a 5-slot mailbox (50-byte max) and spawns two children:
*   - Child1 (priority 4): Sends 6 messages (blocking)
*   - Child2 (priority 3): Receives 6 messages
*
* Child1 runs first, fills all 5 slots, then blocks on the 6th send because
* the mailbox is full. Child2 (higher priority) then runs and begins
* receiving. After receiving the first message, a slot opens and Child1 is
* unblocked to deliver its 6th message. Child2 continues receiving all 6.
*
* Tests the blocking send behavior when a mailbox is full and the wake-up
* of a blocked sender when a slot becomes available.
*
* Expected: All 6 messages delivered and received in order.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
    char* testName = GetTestName(__FILE__);
    int status, kidpid;
    char childNames[MAXPROC][256];
    char nameBuffer[512];
    int childId = 0;
    int mailboxId;
    char* optionSeparator;


    memset(childNames, 0, sizeof(childNames));

    console_output(FALSE, "\n%s: started\n", testName);

    mailboxId = mailbox_create(5, 50);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);

    /* 6 Sends - 0 Receives - No Options */
    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 6, 0, OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    /* 0 Sends - 6 Receives - No Options */
    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 0, 6, OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    k_exit(0);

    return 0;
}
