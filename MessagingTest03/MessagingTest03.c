
#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

/*********************************************************************************
*
* MessagingTest03 - Single-Process Send and Receive
*
* Creates a mailbox (10 slots, 50-byte max) and performs a blocking send
* followed by a blocking receive in the same process. The message "hello there"
* is sent and then retrieved.
*
* Since the mailbox has available slots, the send completes immediately without
* blocking. The receive then retrieves the message from the mailbox.
*
* Expected: Send returns 0 (success). Receive returns the message size and
*           the received buffer contains "hello there".
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
	int mailboxId;
	char* testName = GetTestName(__FILE__);
	int result;
	char buffer[80];

	memset(buffer, 0, sizeof(buffer));
	console_output(FALSE, "\n%s: started\n", testName);

	mailboxId = mailbox_create(10, 50);
	console_output(FALSE, "%s: mailbox_create returned id = %d\n", testName, mailboxId);

	console_output(FALSE, "%s: sending message to mailbox %d\n", testName, mailboxId);
	result = mailbox_send(mailboxId, "hello there", 12, TRUE);
	console_output(FALSE, "%s: after send of message, result = %d\n", testName, result);

	console_output(FALSE, "%s: receiving message from mailbox %d\n", testName, mailboxId);
	result = mailbox_receive(mailboxId, buffer, sizeof(buffer), TRUE);
	console_output(FALSE, "%s: after receipt of message, result = %d\n", testName, result);
	console_output(FALSE, "%s:   message received = %s\n", testName, buffer);

	k_exit(0);

	return 0;
}
