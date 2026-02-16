#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

/*********************************************************************************
*
* MessagingTest07 - Multiple Blocked Senders with Single Receiver
*
* Creates a 5-slot mailbox (50-byte max) and spawns five children:
*   - Child1 (priority 3): Sends 5 messages, filling the mailbox
*   - Child2 (priority 3): Sends 1 message (message #5) - blocks (mailbox full)
*   - Child3 (priority 3): Sends 1 message (message #6) - blocks (mailbox full)
*   - Child4 (priority 3): Sends 1 message (message #7) - blocks (mailbox full)
*   - Child5 (priority 2): Receives 8 messages
*
* After Child1 fills the 5 slots, Children 2-4 each attempt to send and block
* because no slots are available. Child5 (highest priority among children)
* begins receiving. As each message is received and a slot frees up, one
* blocked sender is woken to deliver its message.
*
* Tests multiple blocked senders being unblocked in FIFO order as slots
* become available, and that message ordering is preserved.
*
* Expected: All 8 messages received in send order (Message Number 0-7).
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
    int messageCount = 0;


    memset(childNames, 0, sizeof(childNames));

    console_output(FALSE, "\n%s: started\n", testName);

    mailboxId = mailbox_create(5, 50);
    console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mailboxId);

    /* 5 Sends - 0 Receives - No Options */
    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 5, 0, OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);
    messageCount = 5; 

    /* 1 Sends - 0 Receives - No Options */
    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 1, 0, (messageCount << 8) | OPTION_START_NUMBER | OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);
    messageCount++;

    /* 1 Sends - 0 Receives - No Options */
    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 1, 0, (messageCount << 8) | OPTION_START_NUMBER | OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);
    messageCount++;

    /* 1 Sends - 0 Receives - No Options */
    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 1, 0, (messageCount << 8) | OPTION_START_NUMBER | OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    /* 0 Sends - 8 Receives - No Options */
    optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 0, 8, OPTION_NONE);
    kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 2);
    optionSeparator[0] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

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
}
