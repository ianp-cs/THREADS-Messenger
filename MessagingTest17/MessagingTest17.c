#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

int SendOneAndExit(char* strArgs);
int CloseMailbox(char* strArgs);
int mailboxId;

char childNames[MAXPROC][256];



/*********************************************************************************
*
* MessagingTest17 - Mailbox Free with Blocked Senders on Zero-Slot Mailbox
*
* Creates a zero-slot mailbox (0 slots, 50-byte max) and spawns:
*   - Child1-3 (priority 3): Each sends 1 message (blocking). Since the
*     mailbox has 0 slots and no receiver, all three block on send.
*   - Child4 (priority 3): SimpleDelayExit - exits quickly after a delay.
*   - Child5 (priority 4): Frees the mailbox via mailbox_free.
*
* The parent waits for Child4 first to verify k_wait ordering (Child4
* should exit before blocked children). Then Child5 frees the mailbox,
* unblocking all three senders with a release signal (-5).
*
* Tests mailbox_free on a zero-slot mailbox with multiple blocked senders.
*
* Expected: Child4 exits first. mailbox_free unblocks Child1-3.
*           All children exit with -3.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
    int status, kidpid, pausepid;
    char* testName = GetTestName(__FILE__);
    char nameBuffer[512];

    console_output(FALSE, "\n%s: started\n", testName);

    mailboxId = mailbox_create(0, 50);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);

    /* Use the -Child naming convention for the child process name. */
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", testName);
    kidpid = k_spawn(nameBuffer, SendOneAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    strncpy(childNames[kidpid], nameBuffer, 256);

    /* Use the -Child naming convention for the child process name. */
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child2", testName);
    kidpid = k_spawn(nameBuffer, SendOneAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    strncpy(childNames[kidpid], nameBuffer, 256);

    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child3", testName);
    kidpid = k_spawn(nameBuffer, SendOneAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    strncpy(childNames[kidpid], nameBuffer, 256);

    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child4", testName);
    pausepid = k_spawn(nameBuffer, SimpleDelayExit, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    strncpy(childNames[pausepid], nameBuffer, 256);

    kidpid = k_wait(&status);
    if (kidpid != pausepid)
    {
        console_output(FALSE, "%s: ***Test Failed*** -- join with pausepid failed!\n", testName);
    }
    else
    {
        console_output(FALSE, "%s: Exit status for %s is %d\n", testName, childNames[kidpid], status);
    }

    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child5", testName);
    kidpid = k_spawn(nameBuffer, CloseMailbox, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
    strncpy(childNames[kidpid], nameBuffer, 256);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for %s is %d\n", testName, childNames[kidpid], status);

    k_exit(0);

    return 0;
} /* MessagingEntryPoint */




int SendOneAndExit(char* strArgs)
{
    int result;
    char buffer[32];

    console_output(FALSE, "%s: started\n", strArgs);
    console_output(FALSE, "%s: Sending message to mailbox %d\n", strArgs, mailboxId);
    sprintf(buffer, "Hello from %d", k_getpid());
    result = mailbox_send(mailboxId, buffer, (int)strlen(buffer) + 1, TRUE);
    if (result == 0)
    {
        console_output(FALSE, "%s: mailbox_send Returned %d - Message '%s' DELIVERED\n", strArgs, result, buffer);
    }

    k_exit(-3);

    return 0;
} /* SendSixAndExit */

int CloseMailbox(char* strArgs)
{
    int result;

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);

        result = mailbox_free(mailboxId);

        console_output(FALSE, "%s: mailbox_free returned %d\n", strArgs, result);

    }

    k_exit(-3);

    return 0;
} /* CloseMailbox */
