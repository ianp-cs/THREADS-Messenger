#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

int SendEightWaitSendEight(char* strArgs);
int ReceiveFourSendZeroReceiveFour(char* strArgs);

int mailboxId;
int privateMailboxId;

/*********************************************************************************
*
* MessagingTest11 - Flow Control with Private Zero-Slot Mailbox
*
* Creates two mailboxes:
*   - Main mailbox (5 slots, 50-byte max): For message passing
*   - Private mailbox (0 slots, 50-byte max): For synchronization
*
* Spawns two children:
*   - Child1 (priority 4): Non-blocking sends 8 messages (5 succeed, 3 fail),
*     then waits on private mailbox, then non-blocking sends 8 more.
*   - Child2 (priority 3): Blocking receives 5 messages, signals Child1 via
*     private mailbox, then blocking receives 5 more.
*
* Tests non-blocking send behavior when mailbox is full (returns -2 for
* messages 5-7), zero-slot mailbox used for process synchronization, and
* two rounds of send/receive coordinated by the private mailbox signal.
*
* Expected: First round: 5 delivered, 3 fail. Second round: 5 delivered,
*           3 fail. Child2 receives 10 total messages.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
    int status, kidpid;
    char* testName = GetTestName(__FILE__);
    char nameBuffer[512];
    char childNames[MAXPROC][256];

	console_output(FALSE, "\n%s: started\n", testName);
	
    mailboxId = mailbox_create(5, 50);
	console_output(FALSE, "%s: mailbox_create returned id = %d\n", testName, mailboxId);
	
    privateMailboxId = mailbox_create(0, 50);
	console_output(FALSE, "%s: mailbox_create returned id = %d\n", testName, privateMailboxId);

    /* Use the -Child naming convention for the child process name. */
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", testName);
    kidpid = k_spawn(nameBuffer, SendEightWaitSendEight, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
    strncpy(childNames[kidpid], nameBuffer, 256);

    /* Use the -Child naming convention for the child process name. */
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child2", testName);
    kidpid = k_spawn(nameBuffer, ReceiveFourSendZeroReceiveFour, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    strncpy(childNames[kidpid], nameBuffer, 256);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

	k_exit(0);
	return 0; 
}

int SendEightWaitSendEight(char* strArgs)
{
    int i, result;
    char outMessage[100];

    console_output(FALSE, "%s: started\n", strArgs);

    for (i = 0; i < 8; i++) {

        console_output(FALSE, "%s: Sending message %d to mailbox %d\n", strArgs, i, mailboxId);
        sprintf(outMessage, "Hello There %d", i);
        result = mailbox_send(mailboxId, outMessage, (int)strlen(outMessage) + 1, FALSE);
        if (result == 0)
        {
            console_output(FALSE, "%s: mailbox_send Returned %d - Message '%s' DELIVERED\n", strArgs, result, outMessage);
        }
        else
        {
            console_output(FALSE, "%s: mailbox_send Returned %d\n", strArgs, result);
        }
    }

    mailbox_receive(privateMailboxId, NULL, 0, TRUE);

    for (i = 8; i < 16; i++) {

        console_output(FALSE, "%s: Sending message %d to mailbox %d\n", strArgs, i, mailboxId);
        sprintf(outMessage, "Hello There %d", i);
        result = mailbox_send(mailboxId, outMessage, (int)strlen(outMessage) + 1, FALSE);
        if (result == 0)
        {
            console_output(FALSE, "%s: mailbox_send Returned %d - Message '%s' DELIVERED\n", strArgs, result, outMessage);
        }
        else
        {
            console_output(FALSE, "%s: mailbox_send Returned %d\n", strArgs, result);
        }
    }

    k_exit(-3);

    return 0;
}


int ReceiveFourSendZeroReceiveFour(char* strArgs)
{
    char message[100];
    int i, result;

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);

        for (i = 0; i < 5; i++) {
            console_output(FALSE, "%s: Receiving message %d from mailbox %d\n", strArgs, i, mailboxId);
            result = mailbox_receive(mailboxId, message, sizeof(message), TRUE);
            console_output(FALSE, "%s: Received message %d, result = %d, message = %s\n", strArgs, i, result, message);
        }

        mailbox_send(privateMailboxId, NULL, 0, TRUE);

        for (i = 0; i < 5; i++) {
            console_output(FALSE, "%s: Receiving message %d from mailbox %d\n", strArgs, i, mailboxId);
            result = mailbox_receive(mailboxId, message, sizeof(message), TRUE);
            console_output(FALSE, "%s: Received message %d, result = %d, message = %s\n", strArgs, i, result, message);
        }
    }

    k_exit(-3);

    return 0;

}
