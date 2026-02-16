#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

/*********************************************************************************
*
* MessagingTest01 - Basic Mailbox Creation
*
* Creates two mailboxes with different slot counts and slot sizes:
*   - Mailbox 1: 10 slots, 50-byte messages
*   - Mailbox 2: 20 slots, 30-byte messages
*
* Verifies that mailbox_create returns valid (non-negative) mailbox IDs and
* that multiple mailboxes can be created independently.
*
* Expected: Two successful mailbox_create calls returning distinct IDs >= 0.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
	int mbox_id;
	char* testName = GetTestName(__FILE__);

	console_output(FALSE, "\n%s: started\n", testName);
	
	mbox_id = mailbox_create(10, 50);
	console_output(FALSE, "%s: mailbox_create returned id = %d\n", testName, mbox_id);
	
	mbox_id = mailbox_create(20, 30);
	console_output(FALSE, "%s: mailbox_create returned id = %d\n", testName, mbox_id);

	k_exit(0);

	return 0; 
}
