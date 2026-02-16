#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

int SendTwoAndExit(char* arg);
int ReceiveTwoAndExit(char* arg);
int mailboxId;
char childNames[MAXPROC][256];


/*********************************************************************************
*
* MessagingTest22 - Zero-Slot Mailbox: Sender Higher Priority than Receiver
*
* Creates a zero-slot mailbox (0 slots, 50-byte max) and spawns:
*   - Child1 (priority 3): Sends 2 messages (blocking)
*   - Child2 (priority 4): Receives 2 messages (blocking)
*
* Child1 (higher priority) runs first and blocks on send (no receiver and
* no slots). Child2 then runs and receives directly from the waiting sender
* via the zero-slot rendezvous. This repeats for the second message.
*
* Mirrors Test21 but with reversed priorities: the sender blocks first
* waiting for a receiver, and the receiver picks up the message from the
* waiting sender's slot.
*
* Expected: Two messages sent and received via sender-waits-for-receiver
*           handoff. Both children exit with -3.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
    char* testName = GetTestName(__FILE__);
    int status, kidpid;

    char nameBuffer[512];

    console_output(FALSE, "\n%s: started\n", testName);

    mailboxId = mailbox_create(0, 50);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);

    /* Use the -Child naming convention for the child process name. */
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", testName);

    kidpid = k_spawn(nameBuffer, SendTwoAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    strncpy(childNames[kidpid], nameBuffer, 256);

    /* Use the -Child naming convention for the child process name. */
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child2", testName);
    kidpid = k_spawn(nameBuffer, ReceiveTwoAndExit, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
    strncpy(childNames[kidpid], nameBuffer, 256);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    k_exit(0);

    console_output(FALSE, "\n%s: started\n", testName);


    return 0;
} /* MessagingEntryPoint */


int SendTwoAndExit(char* strArgs)
{
    int i, result;
    char buffer[20];

    console_output(FALSE, "%s: started\n", strArgs);
    for (i = 0; i < 2; i++) {

        console_output(FALSE, "%s: Sending message %d to mailbox %d\n", strArgs, i, mailboxId);
        sprintf(buffer, "Hello There, %d", i);
        result = mailbox_send(mailboxId, buffer, (int)strlen(buffer) + 1, TRUE);
        console_output(FALSE, "%s: Delivered message - result = %d, message = %s\n", strArgs, result, buffer);
    }

    k_exit(-3);

    return 0;
}


int ReceiveTwoAndExit(char* strArgs)
{
    int i, result;
    char message[100];

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);

        for (i = 0; i < 2; i++) {
            console_output(FALSE, "%s: Receiving message %d from mailbox %d\n", strArgs, i, mailboxId);
            result = mailbox_receive(mailboxId, message, sizeof(message), TRUE);
            console_output(FALSE, "%s: Received message %d, result = %d, message = %s\n", strArgs, i, result, message);
        }
    }

    k_exit(-3);

    return 0;
}
