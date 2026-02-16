#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"


/*********************************************************************************
*
* MessagingTest25 - Mailbox Free and Reuse
*
* Creates MAXMBOX - 5 mailboxes, saves IDs for mailboxes at indices 20-29
* (10 mailboxes). Then frees those 10 mailboxes using mailbox_free.
*
* After freeing, attempts to create 14 new mailboxes. The first 10 should
* succeed (reusing freed slots) and the next 4 should also succeed (using
* remaining slots from the original 5 left open). Creates beyond capacity
* should fail with id < 0.
*
* Tests that mailbox_free properly returns mailbox slots to the pool and
* that freed mailbox IDs can be reused by subsequent mailbox_create calls.
*
* Expected: 10 frees succeed. 14 new creates mostly succeed. Overflow
*           creates return id < 0.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
	int mbox_id;
    int i;
	char* testName = GetTestName(__FILE__);
    int tenMailboxes[10];
    int idCount = 0;

	console_output(FALSE, "\n%s: started\n", testName);

    console_output(FALSE, "\n%s: trying to create too many mailboxes\n", testName);

    for (i = 0; i < MAXMBOX - 5; i++) 
    {
        mbox_id = mailbox_create(10, 50);
        if (mbox_id < 0)
        {
            console_output(FALSE, "\n%s: mailbox_create returned id less than zero, id = %d\n", testName, mbox_id);
        }
        else
        {
            if (i >= 20 && i < 30)
            {
                tenMailboxes[i - 20] = mbox_id;
                idCount++;
            }
        }
    }

    for (i = 0; i < idCount; i++)
    {
        int result;

        result = mailbox_free(tenMailboxes[i]);
        console_output(FALSE, "\n%s: mailbox_free returned %d\n", testName, result);
    }

    for (i = 0; i < 14; i++)
    {
        mbox_id = mailbox_create(10, 50);
        if (mbox_id < 0)
        {
            console_output(FALSE, "\n%s: mailbox_create returned id less than zero, id = %d\n", testName, mbox_id);
        }
        else
        {
            console_output(FALSE, "\n%s: mailbox_create returned id = %d\n", testName, mbox_id);
        }
    }

	k_exit(0);
	return 0; 
}
