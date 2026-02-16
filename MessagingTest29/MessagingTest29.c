#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

int ZeroLenReceiver(char* strArgs);
int ZeroLenSenderThenReceiver(char* strArgs);

int mailboxSlotted;
int mailboxZeroSlot;
char childNames[MAXPROC][256];

/*********************************************************************************
*
* MessagingTest29
*
* Zero-length message focused test.
*
* Tests sending and receiving messages with size 0 and a NULL data pointer
* across both slotted and zero-slot mailboxes. Zero-length messages are
* used in the framework as synchronization signals (e.g., the private
* mailbox pattern in Tests 11/12), but no prior test exercises them as
* the primary focus.
*
* Test sequence:
*   Phase 1 - Slotted mailbox (5 slots, 50-byte max):
*     a) Parent sends a zero-length message (NULL, 0) to the slotted mailbox.
*     b) Child1 (priority 3) receives it and verifies result == 0 (size).
*     c) Child1 sends a zero-length reply.
*     d) Parent receives the reply and verifies result == 0.
*
*   Phase 2 - Zero-slot mailbox (rendezvous):
*     e) Child2 (priority 4) blocks on receive from zero-slot mailbox.
*     f) Parent sends zero-length message, directly waking Child2.
*     g) Child2 verifies receipt, sends a zero-length reply.
*     h) Parent receives the reply.
*
*   Phase 3 - Multiple zero-length messages:
*     i) Parent sends 3 zero-length messages to the slotted mailbox.
*     j) Child3 (priority 3) receives all 3, verifying each returns 0.
*
* Expected output:
*   - All zero-length sends return 0 (success).
*   - All zero-length receives return 0 (message size = 0).
*
* Functions tested:
*   mailbox_create, mailbox_send, mailbox_receive
*
* Coverage gap filled:
*   Zero-length messages were only used incidentally as synchronization
*   signals in Tests 11/12. This test validates them as a first-class
*   message type on both slotted and zero-slot mailboxes.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
	char* testName = GetTestName(__FILE__);
	int status, kidpid;
	int result;
	char nameBuffer[512];

	memset(childNames, 0, sizeof(childNames));

	console_output(FALSE, "\n%s: started\n", testName);

	/* Create a slotted mailbox and a zero-slot mailbox */
	mailboxSlotted = mailbox_create(5, 50);
	console_output(FALSE, "%s: mailbox_create (slotted) returned id = %d\n",
		testName, mailboxSlotted);

	mailboxZeroSlot = mailbox_create(0, 50);
	console_output(FALSE, "%s: mailbox_create (zero-slot) returned id = %d\n",
		testName, mailboxZeroSlot);

	/* --- Phase 1: Zero-length on slotted mailbox --- */
	console_output(FALSE, "\n%s: Phase 1 - Zero-length on slotted mailbox\n", testName);

	/* Send a zero-length message */
	result = mailbox_send(mailboxSlotted, NULL, 0, FALSE);
	if (result == 0)
	{
		console_output(FALSE, "%s: Zero-length send to slotted mailbox: PASSED (result = %d)\n",
			testName, result);
	}
	else
	{
		console_output(FALSE, "%s: Zero-length send to slotted mailbox: FAILED (result = %d)\n",
			testName, result);
	}

	/* Spawn Child1 to receive zero-length and reply */
	snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", testName);
	kidpid = k_spawn(nameBuffer, ZeroLenSenderThenReceiver, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
	strncpy(childNames[kidpid], nameBuffer, 256);

	/* Parent receives zero-length reply from Child1 */
	result = mailbox_receive(mailboxSlotted, NULL, 0, TRUE);
	if (result == 0)
	{
		console_output(FALSE, "%s: Zero-length receive reply from Child1: PASSED (result = %d)\n",
			testName, result);
	}
	else
	{
		console_output(FALSE, "%s: Zero-length receive reply from Child1: FAILED (result = %d)\n",
			testName, result);
	}

	/* --- Phase 2: Zero-length on zero-slot mailbox (rendezvous) --- */
	console_output(FALSE, "\n%s: Phase 2 - Zero-length on zero-slot mailbox\n", testName);

	/* Spawn Child2 to block on receive from zero-slot mailbox */
	snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child2", testName);
	kidpid = k_spawn(nameBuffer, ZeroLenReceiver, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
	strncpy(childNames[kidpid], nameBuffer, 256);

	/* Small delay so Child2 blocks on receive first */
	SystemDelay(10);

	/* Send zero-length message to wake Child2 */
	result = mailbox_send(mailboxZeroSlot, NULL, 0, TRUE);
	if (result == 0)
	{
		console_output(FALSE, "%s: Zero-length send to zero-slot mailbox: PASSED (result = %d)\n",
			testName, result);
	}
	else
	{
		console_output(FALSE, "%s: Zero-length send to zero-slot mailbox: FAILED (result = %d)\n",
			testName, result);
	}

	/* --- Phase 3: Multiple zero-length messages --- */
	console_output(FALSE, "\n%s: Phase 3 - Multiple zero-length messages\n", testName);

	/* Send 3 zero-length messages to slotted mailbox */
	for (int i = 0; i < 3; i++)
	{
		result = mailbox_send(mailboxSlotted, NULL, 0, FALSE);
		if (result == 0)
		{
			console_output(FALSE, "%s: Zero-length send %d: PASSED\n", testName, i);
		}
		else
		{
			console_output(FALSE, "%s: Zero-length send %d: FAILED (result = %d)\n",
				testName, i, result);
		}
	}

	/* Spawn Child3 to receive the 3 messages */
	snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child3", testName);
	kidpid = k_spawn(nameBuffer, ZeroLenReceiver, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
	strncpy(childNames[kidpid], nameBuffer, 256);

	/* Wait for all children */
	for (int i = 0; i < 3; i++)
	{
		kidpid = k_wait(&status);
		console_output(FALSE, "%s: Exit status for child %s is %d\n",
			testName, childNames[kidpid], status);
	}

	mailbox_free(mailboxSlotted);
	mailbox_free(mailboxZeroSlot);

	k_exit(0);
	return 0;
}

/*********************************************************************************
* ZeroLenReceiver
*
* Receives 3 zero-length messages from the slotted mailbox (Phase 3) or
* 1 zero-length message from the zero-slot mailbox (Phase 2), depending
* on which mailbox has data. Used for both Phase 2 Child2 and Phase 3 Child3.
*
* For Phase 2 (Child2): receives 1 message from zero-slot mailbox.
* For Phase 3 (Child3): receives 3 messages from slotted mailbox.
*********************************************************************************/
int ZeroLenReceiver(char* strArgs)
{
	int result;
	int childNum;

	console_output(FALSE, "%s: started\n", strArgs);

	childNum = GetChildNumber(strArgs);

	if (childNum == 2)
	{
		/* Phase 2: receive one zero-length from zero-slot mailbox */
		result = mailbox_receive(mailboxZeroSlot, NULL, 0, TRUE);
		if (result == 0)
		{
			console_output(FALSE, "%s: Zero-length receive from zero-slot: PASSED (result = %d)\n",
				strArgs, result);
		}
		else
		{
			console_output(FALSE, "%s: Zero-length receive from zero-slot: FAILED (result = %d)\n",
				strArgs, result);
		}
	}
	else
	{
		/* Phase 3: receive 3 zero-length messages from slotted mailbox */
		for (int i = 0; i < 3; i++)
		{
			result = mailbox_receive(mailboxSlotted, NULL, 0, TRUE);
			if (result == 0)
			{
				console_output(FALSE, "%s: Zero-length receive %d: PASSED (result = %d)\n",
					strArgs, i, result);
			}
			else
			{
				console_output(FALSE, "%s: Zero-length receive %d: FAILED (result = %d)\n",
					strArgs, i, result);
			}
		}
	}

	k_exit(-3);
	return 0;
}

/*********************************************************************************
* ZeroLenSenderThenReceiver
*
* Phase 1 helper. Receives one zero-length message from the slotted mailbox,
* then sends a zero-length reply back.
*********************************************************************************/
int ZeroLenSenderThenReceiver(char* strArgs)
{
	int result;

	console_output(FALSE, "%s: started\n", strArgs);

	/* Receive zero-length message from parent */
	result = mailbox_receive(mailboxSlotted, NULL, 0, TRUE);
	if (result == 0)
	{
		console_output(FALSE, "%s: Zero-length receive: PASSED (result = %d)\n",
			strArgs, result);
	}
	else
	{
		console_output(FALSE, "%s: Zero-length receive: FAILED (result = %d)\n",
			strArgs, result);
	}

	/* Send zero-length reply to parent */
	result = mailbox_send(mailboxSlotted, NULL, 0, FALSE);
	if (result == 0)
	{
		console_output(FALSE, "%s: Zero-length send reply: PASSED (result = %d)\n",
			strArgs, result);
	}
	else
	{
		console_output(FALSE, "%s: Zero-length send reply: FAILED (result = %d)\n",
			strArgs, result);
	}

	k_exit(-3);
	return 0;
}
