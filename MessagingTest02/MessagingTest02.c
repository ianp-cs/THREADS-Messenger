#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

/*********************************************************************************
*
* MessagingTest02 - Mailbox Exhaustion
*
* Attempts to create more than MAXMBOX mailboxes to test the mailbox table
* overflow condition. Creates MAXMBOX + 3 mailboxes in a loop.
*
* The first MAXMBOX creations should succeed; the final 3 should fail and
* return a negative ID, which is printed as an error.
*
* Expected: First MAXMBOX calls succeed. Last 3 return id < 0.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
	int mbox_id;
	char* testName = GetTestName(__FILE__);


	console_output(FALSE, "\n%s: started\n", testName);
	
	for (int i = 0; i < MAXMBOX + 3; i++) 
	{
		mbox_id = mailbox_create(10, 50);
		if (mbox_id < 0)
		{
			console_output(FALSE, "%s: mailbox_create returned id less than zero, id = %d\n",
				testName, mbox_id);
		}
	}

	k_exit(0);
	return 0; 
}
