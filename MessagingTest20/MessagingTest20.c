#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

int ReceiveOneAndExit(char* strArgs);
int CloseMailbox(char* strArgs);
int mailboxId;

char childNames[MAXPROC][256];


/*********************************************************************************
*
* MessagingTest20 - Receive from Released Mailbox
*
* Creates a zero-slot mailbox and spawns:
*   - Child1-3 (priority 3): Each receives 1 message (blocking) - all block.
*   - Child4 (priority 3): SimpleDelayExit - exits after a delay.
*   - Child5 (priority 4): Frees the mailbox via mailbox_free.
*
* After all children complete, the parent attempts a non-blocking receive
* from the now-freed mailbox. This should return -1 since the mailbox is
* no longer valid.
*
* Mirrors Test19 but tests receive on a released mailbox instead of send.
*
* Expected: Child4 exits first. mailbox_free unblocks Child1-3.
*           Parent's receive on released mailbox returns -1 (SUCCESS).
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
    int status, kidpid, pausepid, result;
    char* testName = GetTestName(__FILE__);
    char nameBuffer[512];
    char message[64];

    console_output(FALSE, "\n%s: started\n", testName);

    mailboxId = mailbox_create(0, 50);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);

    /* Use the -Child naming convention for the child process name. */
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", testName);
    kidpid = k_spawn(nameBuffer, ReceiveOneAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    strncpy(childNames[kidpid], nameBuffer, 256);

    /* Use the -Child naming convention for the child process name. */
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child2", testName);
    kidpid = k_spawn(nameBuffer, ReceiveOneAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    strncpy(childNames[kidpid], nameBuffer, 256);

    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child3", testName);
    kidpid = k_spawn(nameBuffer, ReceiveOneAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
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

    sprintf(message, "Released mailbox receive test");
    result = mailbox_receive(mailboxId, message, strlen(message) + 1, FALSE);

    if (result == -1)
        console_output(FALSE, "%s: Message receive failed on released mailbox - SUCCESS\n", testName);
    else
        console_output(FALSE, "%s: Message receive returned incorrect on released mailbox - FAIL\n", testName);

    k_exit(0);

    return 0;
} /* MessagingEntryPoint */




int ReceiveOneAndExit(char* strArgs)
{
    int result;
    char buffer[32];

    console_output(FALSE, "%s: started\n", strArgs);
    console_output(FALSE, "%s: Receiving message from mailbox %d\n", strArgs, mailboxId);
    result = mailbox_receive(mailboxId, buffer, sizeof(buffer), TRUE);
    if (result > 0)
    {
        console_output(FALSE, "%s: mailbox_receive Returned %d - Message '%s' RECEIVED\n", strArgs, result, buffer);
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
