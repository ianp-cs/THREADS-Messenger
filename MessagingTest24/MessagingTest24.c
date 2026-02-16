#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

int mailboxId;
int SendFiveSpawnFour(char* strArgs);
int SendOneAndExit(char* strArgs);
int ReceiveEightAndExit(char* strArgs);

/*********************************************************************************
*
* MessagingTest24 - Full Mailbox with Blocked Senders and Receiver Drain
*
* Creates a mailbox (5 slots, MAX_MESSAGE) and spawns a priority-1 child
* that:
*   1. Sends 5 messages, filling the mailbox.
*   2. Spawns three senders at priorities 2, 3, 4 that each send 1 message.
*      Since the mailbox is full, all three block on send.
*   3. Spawns a receiver at priority 5 that receives all 8 messages.
*
* As the receiver consumes messages, slots open up, unblocking the waiting
* senders in FIFO order. Each unblocked sender's message is placed into
* the deliveredMail queue, maintaining send order.
*
* Tests the interaction between a full mailbox, multiple blocked senders at
* different priorities, and a single receiver draining the mailbox.
*
* Expected: All 8 messages received in order (Message Number 0-7).
*           Blocked senders unblocked as slots become available.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
    char* testName = GetTestName(__FILE__);
    int status, kidpid;
    char childNames[MAXPROC][256];
    char nameBuffer[512];

    memset(childNames, 0, sizeof(childNames));

    console_output(FALSE, "\n%s: started\n", testName);

    mailboxId = mailbox_create(5, MAX_MESSAGE);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);

    /* Use the -Child naming convention for the child process name. */
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", testName);
    kidpid = k_spawn(nameBuffer, SendFiveSpawnFour, nameBuffer, THREADS_MIN_STACK_SIZE, 1);
    strncpy(childNames[kidpid], nameBuffer, 256);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    k_exit(0);

    console_output(FALSE, "\n%s: started\n", testName);


    return 0;
} /* MessagingEntryPoint */


int SendFiveSpawnFour(char* strArgs)
{
    int i, result, kidpid, status;
    char buffer[20];
    char nameBuffer[512];
    char childNames[MAXPROC][256];
    memset(childNames, 0, sizeof(childNames));

    console_output(FALSE, "%s: started\n", strArgs);
    for (i = 0; i < 5; i++) {

            sprintf(buffer, "Message Number %d", i);
        result = mailbox_send(mailboxId, buffer, (int)strlen(buffer) + 1, TRUE);
        console_output(FALSE, "%s: Delivered message: %s\n", strArgs, buffer);
    }

    console_output(FALSE, "%s: Spawning SendOneAndExit at priority 2\n", strArgs);
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", strArgs);
    kidpid = k_spawn(nameBuffer, SendOneAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 2);
    strncpy(childNames[kidpid], nameBuffer, 256);

    console_output(FALSE, "%s: Spawning SendOneAndExit at priority 3\n", strArgs);
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child2", strArgs);
    kidpid = k_spawn(nameBuffer, SendOneAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    strncpy(childNames[kidpid], nameBuffer, 256);

    console_output(FALSE, "%s: Spawning SendOneAndExit at priority 4\n", strArgs);
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child3", strArgs);
    kidpid = k_spawn(nameBuffer, SendOneAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
    strncpy(childNames[kidpid], nameBuffer, 256);

    console_output(FALSE, "%s: Spawning ReceiveEightAndExit at priority 5\n", strArgs);
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child4", strArgs);
    kidpid = k_spawn(nameBuffer, ReceiveEightAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 5);
    strncpy(childNames[kidpid], nameBuffer, 256);

    for (i = 0; i < 4; ++i)
    {
        kidpid = k_wait(&status);
        console_output(FALSE, "%s: Exit status for child %s is %d\n", strArgs, childNames[kidpid], status);
    }

    k_exit(-3);

    return 0;
}

int SendOneAndExit(char* strArgs)
{
    int result;
    char message[100];
    int testNumber;

    testNumber = GetChildNumber(strArgs);
    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);

        sprintf(message, "Message Number %d", testNumber + 4);
        result = mailbox_send(mailboxId, message, strlen(message), TRUE);
        console_output(FALSE, "%s: Delivered message: %s\n", strArgs, message);
    }

    k_exit(-3);

    return 0;
}

int ReceiveEightAndExit(char* strArgs)
{
    int result;
    char message[100];

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);
        for (int i = 0; i < 8; ++i)
        {
            result = mailbox_receive(mailboxId, message, sizeof(message), TRUE);
            console_output(FALSE, "%s: message received: %s\n", strArgs, message);
        }

    }

    k_exit(-3);

    return 0;
}
