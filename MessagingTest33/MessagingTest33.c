#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

/*********************************************************************************
*
* MessagingTest33 - Send and Receive at Exact Slot Size
*
* Creates a mailbox with 10 slots and a 13-byte max slot size. Sends the
* message "Test Message" (12 chars + null = 13 bytes), which exactly matches
* the slot size.
*
* Then receives the message and verifies the receive returns the correct
* message size (13).
*
* Tests the boundary condition where the message size equals the mailbox's
* max slot size exactly.
*
* Expected: Send succeeds (result 0). Receive returns 13 (message size).
*           Received message is "Test Message".
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

    mailboxId = mailbox_create(10, 13);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);

    char message[32];
    strcpy(message, "Test Message");

    console_output(FALSE, "\n%s: Sending message of max mailbox size\n", testName);
    int result = mailbox_send(mailboxId, message, 13, TRUE);
    if (result == 0)
    {
        console_output(FALSE, "%s: Delivered message: %s\n", testName, message);
    }
    else
    {
        console_output(FALSE, "%s: Message delivery failed: result = %d\n", testName, result);
    }

    message[0] = '\0';
    result = mailbox_receive(mailboxId, message, sizeof(message), TRUE);
    if (result == 13)
    {
        console_output(FALSE, "%s: Received message: %s\n", testName, message);
    }
    else
    {
        console_output(FALSE, "%s: Message receive failed: result = %d\n", testName, result);
    }


    k_exit(0);



    return 0;
} /* MessagingEntryPoint */
