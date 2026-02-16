#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

/*********************************************************************************
*
* MessagingTest04 - Two-Process Send and Receive (Sender Higher Priority)
*
* Creates a mailbox (10 slots, 50-byte max) and spawns two child processes:
*   - Child1 (priority 4): Sends 1 message
*   - Child2 (priority 3): Receives 1 message
*
* Child1 runs first (higher priority number = lower priority), sends the
* message into a slot, and exits. Child2 then runs, retrieves the message
* from the mailbox, and exits. The parent waits for both children.
*
* Tests basic two-process mailbox communication where the sender deposits
* a message before the receiver runs.
*
* Expected: Child1 delivers message. Child2 receives it. Both exit with -3.
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

    mailboxId = mailbox_create(10, 50);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);

    /* 1 Sends - 0 Receives - No Options */
    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 1, 0, OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    /* 0 Sends - 1 Receives - No Options */
    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 0, 1, OPTION_NONE);
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
