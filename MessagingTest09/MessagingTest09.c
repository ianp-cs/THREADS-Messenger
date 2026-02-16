#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

/*********************************************************************************
*
* MessagingTest09 - Mailbox Free with Blocked Senders and Wait Ordering
*
* Similar to Test08 but adds a verification step for k_wait ordering.
*
* Creates a 5-slot mailbox (50-byte max) and spawns children:
*   - Child1 (priority 3): Sends 5 messages, filling the mailbox
*   - Child2-4 (priority 3): Each sends 1 message - all block (mailbox full)
*   - Child5 (priority 2): Does nothing (0 sends, 0 receives) - exits quickly
*   - Child6 (priority 4): Frees the mailbox (OPTION_FREE_FIRST)
*
* The parent first waits for Child1 to complete, then waits again expecting
* Child5 (the pause process) to be the next to exit. This verifies k_wait
* returns the correct child. Then Child6 frees the mailbox, unblocking
* the three blocked senders.
*
* Tests mailbox_free with blocked senders combined with k_wait ordering
* verification.
*
* Expected: k_wait returns pausePid for the second wait. All blocked
*           senders unblocked. All children exit with -3.
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
	console_output(FALSE, "%s: mailbox_create returned id = %d\n", testName, mailboxId);

	/* 5 Sends - 0 Receives - No Options */
	optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 5, 0, OPTION_NONE);
	kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
	optionSeparator[0] = '\0';
	strncpy(childNames[kidpid], nameBuffer, 256);
	messageCount = 5;

	kidpid = k_wait(&status);
	console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

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

	/* 0 Sends - 0 Receives - No Options */
	optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 0, 0, OPTION_NONE);
	int pausePid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 2);
	optionSeparator[0] = '\0';
	strncpy(childNames[pausePid], nameBuffer, 256);

	kidpid = k_wait(&status);
	if (kidpid != pausePid)
	{
		printf("\n***Test Failed*** -- wait with pausepid failed!\n\n");
	}


	/* 0 Sends - 0 Receives - Close Option */
	optionSeparator = CreateMessageTestArgs(nameBuffer, sizeof(nameBuffer), testName, ++childId, mailboxId, 0, 0, OPTION_FREE_FIRST);
	kidpid = k_spawn(nameBuffer, SendAndReceive, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
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
	console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[pausePid], status);

	k_exit(0);

	return 0;
}
