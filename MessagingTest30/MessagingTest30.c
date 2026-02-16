#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"



/*********************************************************************************
*
* MessagingTest30
*
* Blocked senders on a 1-slot mailbox with mailbox_free and zero-length send.
*
* Creates a 1-slot, 50-byte mailbox. The parent (priority 5) spawns four
* children and then sends a zero-length message (NULL, 0) which fills the
* single slot. Three child processes each attempt to send one message at
* different priorities (2, 3, 4). Since the slot is already full, all three
* block waiting for a slot.
*
* Child4 (priority 2, OPTION_FREE_FIRST) frees the mailbox, which signals
* and unblocks the blocked senders. They receive -5 from mailbox_send.
*
* Test sequence:
*   1) Create 1-slot/50-byte mailbox.
*   2) Spawn Child1 (pri 2), Child2 (pri 3), Child3 (pri 4), Child4 (pri 2).
*   3) Parent sends zero-length message (NULL, 0) — succeeds, fills the slot.
*   4) Parent calls k_wait, children run in priority order.
*   5) Child3 (pri 4): sends 1 message, blocks (slot full).
*   6) Child2 (pri 3): sends 1 message, blocks (slot full).
*   7) Child1 (pri 2): sends 1 message, blocks (slot full).
*   8) Child4 (pri 2, OPTION_FREE_FIRST): frees mailbox, unblocks 1-3.
*   9) Parent waits for all 4 children.
*
* Expected output:
*   - Parent's zero-length send succeeds (result = 0).
*   - Children 1, 2, and 3 report send failure with result = -5 (released).
*   - Child4 reports mailbox_free returned 0.
*
* Functions tested:
*   mailbox_create, mailbox_send (blocking + zero-length),
*   mailbox_free (with blocked senders)
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

    mailboxId = mailbox_create(1, 50);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);

    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 1, 0, OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 2);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 1, 0, OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 1, 0, OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    /* free the mailbox with a lower priority process */
    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 0, 0, OPTION_FREE_FIRST);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 2);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);


    char message[32];
    strcpy(message, "First message");
    int result = mailbox_send(mailboxId, NULL, 0, TRUE);
    if (result == 0)
    {
        console_output(FALSE, "%s: Delivered message: %s\n", testName, message);
    }
    else
    {
        console_output(FALSE, "%s: Message delivery failed: result = %d\n", testName, result);
    }


    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);


    k_exit(0);



    return 0;
} /* MessagingEntryPoint */

