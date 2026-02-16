#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

/*********************************************************************************
*
* MessagingTest16
*
* Global slot pool exhaustion test. Uses two mailboxes so that exhaustion
* must come from the global pool, not per-mailbox limits. The expected
* outcome is that the system halts via stop(1) when the pool is exhausted.
* If this test reaches k_exit, the implementation failed to halt.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
	int bigMailboxId;
	int smallMailboxId;
	char* testName = GetTestName(__FILE__);
	int sendCount = 0;
	char message[32];

	console_output(FALSE, "\n%s: started\n", testName);

	bigMailboxId = mailbox_create(MAXSLOTS, sizeof(message));
	console_output(FALSE, "%s: mailbox_create returned id = %d\n", testName, bigMailboxId);

	smallMailboxId = mailbox_create(5, sizeof(message));
	console_output(FALSE, "%s: mailbox_create returned id = %d\n", testName, smallMailboxId);

	if (bigMailboxId < 0 || smallMailboxId < 0)
	{
		console_output(FALSE, "%s: FAILED - could not create mailbox\n", testName);
		k_exit(1);
		return 1;
	}

	/* Fill the small mailbox to consume slots from the global pool. */
	for (int i = 0; i < 5; ++i)
	{
		sprintf(message, "Small %d", i);
		mailbox_send(smallMailboxId, message, (int)strlen(message) + 1, FALSE);
		sendCount++;
	}

	/* Fill the big mailbox. The system should halt via stop(1) when
	 * the global slot pool is exhausted. */
	while (1)
	{
		sprintf(message, "Slot msg %d", sendCount);
		mailbox_send(bigMailboxId, message, (int)strlen(message) + 1, FALSE);
		sendCount++;

		if (sendCount == MAXSLOTS - 10)
		{
			console_output(FALSE, "%s: Sent %d messages - system should be halting soon\n",
				testName, sendCount);
		}
	}

	/* Should never reach here. */
	console_output(FALSE, "%s: FAILED - system did not halt on slot exhaustion\n", testName);
	k_exit(1);
	return 1;
}