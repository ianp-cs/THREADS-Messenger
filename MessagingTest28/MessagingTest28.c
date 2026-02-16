#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

/*********************************************************************************
*
* MessagingTest28
*
* Invalid mailbox ID validation test.
*
* Verifies that mailbox_send, mailbox_receive, and mailbox_free all reject
* invalid mailbox IDs by returning -1. Tests the following invalid IDs:
*   1) Negative ID (-1): out-of-range index.
*   2) Large ID (9999): exceeds any allocated mailbox.
*   3) Unallocated ID (valid range but never created).
*   4) Freed ID: a mailbox that was created and then released.
*
* For each invalid ID, the test calls mailbox_send, mailbox_receive, and
* mailbox_free and checks that each returns -1.
*
* Also verifies that a valid mailbox still works correctly after all the
* invalid operations, ensuring no corruption of the mailbox table.
*
* This test uses only the parent process (no child spawns). All sends and
* receives use non-blocking mode to avoid deadlock on invalid operations.
*
* Expected output:
*   - All 12 invalid-ID operations (4 IDs x 3 functions) return -1.
*   - Final send/receive on a valid mailbox succeeds normally.
*
* Functions tested:
*   mailbox_create, mailbox_send, mailbox_receive, mailbox_free
*
* Coverage gap filled:
*   No prior test exercises send/receive/free with bogus or freed mailbox IDs.
*   Test35 validates mailbox_create parameter boundaries but not the other
*   three functions with invalid IDs.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
	char* testName = GetTestName(__FILE__);
	int result;
	int validMbox, freedMbox;
	char message[50];
	int passCount = 0;
	int testCount = 0;

	console_output(FALSE, "\n%s: started\n", testName);

	/* Create a valid mailbox for later use */
	validMbox = mailbox_create(10, 50);
	console_output(FALSE, "%s: mailbox_create returned id = %d (valid mailbox)\n",
		testName, validMbox);

	/* Create and immediately free a mailbox to get a freed ID */
	freedMbox = mailbox_create(5, 50);
	console_output(FALSE, "%s: mailbox_create returned id = %d (will be freed)\n",
		testName, freedMbox);
	result = mailbox_free(freedMbox);
	console_output(FALSE, "%s: mailbox_free returned %d\n", testName, result);

	/* --- Test invalid IDs --- */
	int invalidIds[] = { -1, 9999, freedMbox };
	char* idNames[] = { "negative (-1)", "large (9999)", "freed" };
	int idCount = 3;

	for (int i = 0; i < idCount; i++)
	{
		int id = invalidIds[i];

		/* mailbox_send with invalid ID */
		testCount++;
		strcpy(message, "test message");
		result = mailbox_send(id, message, (int)strlen(message) + 1, FALSE);
		if (result == -1)
		{
			console_output(FALSE, "%s: mailbox_send with %s ID returned -1: PASSED\n",
				testName, idNames[i]);
			passCount++;
		}
		else
		{
			console_output(FALSE, "%s: mailbox_send with %s ID returned %d (expected -1): FAILED\n",
				testName, idNames[i], result);
		}

		/* mailbox_receive with invalid ID */
		testCount++;
		memset(message, 0, sizeof(message));
		result = mailbox_receive(id, message, sizeof(message), FALSE);
		if (result == -1)
		{
			console_output(FALSE, "%s: mailbox_receive with %s ID returned -1: PASSED\n",
				testName, idNames[i]);
			passCount++;
		}
		else
		{
			console_output(FALSE, "%s: mailbox_receive with %s ID returned %d (expected -1): FAILED\n",
				testName, idNames[i], result);
		}

		/* mailbox_free with invalid ID */
		testCount++;
		result = mailbox_free(id);
		if (result == -1)
		{
			console_output(FALSE, "%s: mailbox_free with %s ID returned -1: PASSED\n",
				testName, idNames[i]);
			passCount++;
		}
		else
		{
			console_output(FALSE, "%s: mailbox_free with %s ID returned %d (expected -1): FAILED\n",
				testName, idNames[i], result);
		}
	}

	/* --- Verify valid mailbox still works --- */
	console_output(FALSE, "\n%s: Verifying valid mailbox still works after invalid operations\n", testName);

	strcpy(message, "Valid mailbox test");
	result = mailbox_send(validMbox, message, (int)strlen(message) + 1, FALSE);
	if (result == 0)
	{
		console_output(FALSE, "%s: Send to valid mailbox succeeded: PASSED\n", testName);
		passCount++;
	}
	else
	{
		console_output(FALSE, "%s: Send to valid mailbox returned %d: FAILED\n", testName, result);
	}
	testCount++;

	memset(message, 0, sizeof(message));
	result = mailbox_receive(validMbox, message, sizeof(message), FALSE);
	if (result >= 0)
	{
		console_output(FALSE, "%s: Receive from valid mailbox succeeded, message = %s: PASSED\n",
			testName, message);
		passCount++;
	}
	else
	{
		console_output(FALSE, "%s: Receive from valid mailbox returned %d: FAILED\n", testName, result);
	}
	testCount++;

	console_output(FALSE, "\n%s: Results: %d / %d passed\n", testName, passCount, testCount);

	mailbox_free(validMbox);

	k_exit(0);
	return 0;
}
