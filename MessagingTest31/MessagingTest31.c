#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"



/*********************************************************************************
*
* MessagingTest31 - Blocked Receivers with Mailbox Free and Zero-Length Receive
*
* Creates a 1-slot mailbox (13-byte max) and spawns:
*   - Child1 (priority 2): Receives 1 message - blocks (mailbox empty)
*   - Child2 (priority 3): Receives 1 message - blocks
*   - Child3 (priority 4): Receives 1 message - blocks
*   - Child4 (priority 2): Frees the mailbox (OPTION_FREE_FIRST)
*
* The parent then receives a zero-length message (NULL, 0) from the mailbox
* with blocking. Since Child4 may have freed the mailbox, this tests the
* behavior of receive on a mailbox being or already released.
*
* Mirrors Test30 but with blocked receivers instead of senders.
*
* Expected: mailbox_free unblocks Child1-3. Parent receive behavior depends
*           on timing relative to mailbox_free.
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

    mailboxId = mailbox_create(1, 13);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);

    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 0, 1, OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 2);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 0, 1, OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 0, 1, OPTION_NONE);
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
    int result = mailbox_receive(mailboxId, NULL, 0, TRUE);
    if (result == 0)
    {
        console_output(FALSE, "%s: Received message: %s\n", testName, message);
    }
    else
    {
        console_output(FALSE, "%s: Message receive failed: result = %d\n", testName, result);
    }


    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child process is %d\n", testName, status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child process is %d\n", testName, status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child process is %d\n", testName, status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child process is %d\n", testName, status);


    k_exit(0);



    return 0;
} /* MessagingEntryPoint */
