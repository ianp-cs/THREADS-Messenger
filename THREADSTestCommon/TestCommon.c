#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "THREADSLib.h"
#include "TestCommon.h"
#include "Scheduler.h"
#include "Messaging.h"

int gPid = -1;

/* prototypes for functions used across multiple tests. */
int SimpleDelayExit(void* pArgs);
int SpawnTwoPriorityFour(char* pArgs);
int DelayAndDump(char* arg);
int SignalAndJoinTwoLower(char* strArgs);

static testNameBuffer[512];


/*********************************************************************************
*
*  CreateMessageTestArgs
*
*  Builds a combined name/argument string for use with SendAndReceive.
*
*  The output format is:  "<prefix>-Child<childId>:<mailbox>-<sendCount>-<receiveCount>-<options>"
*
*  The portion before the ':' serves as the child process name (for display).
*  The portion after the ':' is parsed by SendAndReceive to determine what
*  operations to perform.
*
*  The caller typically sets separator[0] = '\0' after spawning so that only
*  the name portion is stored in childNames[].
*
*  Parameters:
*    buffer      - output buffer for the combined string
*    bufferSize  - size of the output buffer
*    prefix      - test name prefix (e.g. "MessagingTest07")
*    childId     - child number for this process
*    mailbox     - mailbox ID to operate on
*    sendCount   - number of messages to send
*    receiveCount- number of messages to receive
*    options     - bitmask of OPTION_* flags; upper byte is the start number
*                  when OPTION_START_NUMBER is set
*
*  Returns:
*    Pointer to the ':' separator character within the buffer.
*
*********************************************************************************/
char* CreateMessageTestArgs(char* buffer, int bufferSize, char* prefix, int childId, int mailbox, int sendCount, int receiveCount, int options)
{
    char* separator;

    snprintf(buffer, bufferSize, "%s-Child%d:%d-%d-%d-%d", prefix, childId, mailbox, sendCount, receiveCount, options);

    separator = strchr(buffer, ':');

    return separator;
}


/*********************************************************************************
*
*  SendAndReceive
*
*  General-purpose child process used by most messaging tests. Parses the
*  argument string (created by CreateMessageTestArgs) to determine which
*  mailbox to use and how many sends/receives to perform.
*
*  The argument string format after the ':' is:
*    "<mailboxId>-<sendCount>-<receiveCount>-<options>"
*
*  Execution order:
*    1) If OPTION_FREE_FIRST is set, calls mailbox_free before any sends/receives.
*    2) Performs the first operation loop:
*       - Sends (sendCount messages), unless OPTION_RECEIVE_FIRST is set.
*       - Receives (receiveCount messages), if OPTION_RECEIVE_FIRST is set.
*    3) Switches to the other operation and performs the second loop.
*       (e.g., if it sent first, it receives second.)
*
*  Option flags (may be combined):
*    OPTION_NONE          (0x00) - Default: blocking send, send-first order.
*    OPTION_FREE_FIRST    (0x01) - Call mailbox_free before send/receive loops.
*    OPTION_RECEIVE_FIRST (0x02) - Receive before sending (instead of send-first).
*    OPTION_NON_BLOCKING  (0x04) - Use non-blocking send/receive (wait=FALSE).
*    OPTION_START_NUMBER  (0x80) - Upper byte of options holds the starting
*                                  message number (for sequential numbering
*                                  across multiple child processes).
*
*  Messages are formatted as "Message Number <N>" where N starts at 0
*  (or at the value in the upper byte if OPTION_START_NUMBER is set).
*
*  Exits with code -3 on normal completion. Exit code -5 is set by the
*  test framework when the process is released via mailbox_free.
*
*********************************************************************************/
int SendAndReceive(char* strArgs)
{
    int i, result;
    char message[MAX_MESSAGE];
    char* separator;
    int sendCount = 0;
    int receiveCount = 0;
    int options = 0;
    int bSending=0;
    int mailbox;
    int loopCount;
    int messageStartNumber=0;
    int blocking = TRUE;

    /* parse the args */
    separator = strchr(strArgs, ':');
    separator[0] = '\0';
    separator++;
    sscanf(separator, "%d-%d-%d-%d", &mailbox, &sendCount, &receiveCount, &options);

    console_output(FALSE, "%s: started\n", strArgs);

    if (options & OPTION_NON_BLOCKING)
    {
        blocking = FALSE;
    }

    if (options & OPTION_START_NUMBER)
    {
        /* extract the starting message number from the upper byte */
        messageStartNumber = options >> 8 & 0x000000FF;
    }
    /* see if the free first option is set. */
    if (options & OPTION_FREE_FIRST)
    {
        result = mailbox_free(mailbox);

        console_output(FALSE, "%s: mailbox_free returned %d\n", strArgs, result);
    }

    if (options & OPTION_RECEIVE_FIRST)
    {
        loopCount = receiveCount;
        bSending = FALSE;
    }
    else
    {
        loopCount = sendCount;
        bSending = TRUE;
    }

    /* Two passes: first pass does the primary operation, second does the other. */
    for (int j = 0; j < 2; ++j)
    {
        for (i = 0; i < loopCount; i++)
        {
            if (bSending)
            {
                sprintf(message, "Message Number %d", i + messageStartNumber);
                result = mailbox_send(mailbox, message, (int)strlen(message) + 1, blocking);
                if (result == 0)
                {
                    console_output(FALSE, "%s: Delivered message: %s\n", strArgs, message);
                }
                else
                {
                    console_output(FALSE, "%s: Message delivery failed: result = %d\n", strArgs, result);
                }
            }
            else
            {
                result = mailbox_receive(mailbox, message, MAX_MESSAGE, blocking);
                if (result >= 0)
                {
                    console_output(FALSE, "%s: Received message: %s\n", strArgs, message);
                }
                else
                {
                    console_output(FALSE, "%s: Message receive failed: result = %d\n", strArgs, result);
                }
            }
        }

        /* switch operations for the second pass */
        if (options & OPTION_RECEIVE_FIRST)
        {
            loopCount = sendCount;
            bSending = TRUE;
        }
        else
        {
            loopCount = receiveCount;
            bSending = FALSE;
        }

    }

    k_exit(-3);

    return 0;
}

/*********************************************************************************
*
*  GetTestName
*
*  Extracts the test name from a __FILE__ path by stripping the directory
*  prefix and the ".c" extension. Uses a single static buffer, so only one
*  test name can be active at a time.
*
*  Parameters:
*    filename - typically __FILE__, e.g. "C:\\path\\MessagingTest07.c"
*
*  Returns:
*    Pointer to the static buffer containing the test name,
*    e.g. "MessagingTest07".
*
*********************************************************************************/
char* GetTestName(char* filename)
{
    char* testName;
    
    testName = filename;
    if (strrchr(filename, '\\'))
    {
        testName = strrchr(filename, '\\') + 1;
    }

    strncpy((char *)testNameBuffer, testName, strlen(testName) - 2);

    return (char*)testNameBuffer;
}

/*********************************************************************************
*
*  SystemDelay
*
*  Busy-wait delay for the specified duration in milliseconds. Uses
*  read_clock() for timing. Optimization is disabled to prevent the
*  compiler from eliminating the spin loop.
*
*  Parameters:
*    millisTime - duration to delay in milliseconds
*
*********************************************************************************/
#pragma optimize( "", off )
void SystemDelay(int millisTime)
{
    unsigned int startTime;
    unsigned int currentTime;

    currentTime = startTime = read_clock() / 1000;
    while ((currentTime - startTime) < (unsigned int)millisTime)
    {
        currentTime = read_clock() / 1000;
    }
}
#pragma optimize( "", on )

/*********************************************************************************
*
*  DelayAndDump
*
*  Child process that runs for ~10 seconds. If the trailing number in its
*  name is divisible by 4, it periodically calls display_process_table()
*  (every ~5 seconds starting at 2.5s). All instances exit after 10 seconds.
*
*  Exits with code -(pid).
*
*  Parameters:
*    arg - process name string (used for output and to extract child number)
*
*********************************************************************************/
int DelayAndDump(char* arg)
{
    unsigned long startTime, currentTime;
    int printingThread = 0;
    unsigned long  printAt = 2500;
    unsigned long  stopAt = 10000;
    char* testName = __func__;
    int testNumber;

    console_output(FALSE, "%s: started\n", arg);

    // Get the test number
    testNumber = GetChildNumber(arg);

    console_output(FALSE, "%s: started, child number is %d\n", arg, testNumber);

    if ((testNumber % 4) == 0)
    {
        printingThread = 1;
    }

    startTime = read_clock() / 1000;
    while (1)
    {
        currentTime = read_clock() / 1000;

        if ((printingThread) && (currentTime - startTime) > printAt)
        {
            printAt += 5000;
            display_process_table();
        }
        else if ((currentTime - startTime) > stopAt)
        {
            break;
        }

    }
    if (strlen(arg) <= 0)
    {
        console_output(FALSE, "NO STRING: %d, %d\n", k_getpid(), testNumber);
    }
    console_output(FALSE, "%s: exiting, pid = %d\n", arg, k_getpid());
    k_exit(-k_getpid());
    return 0;
}


/*********************************************************************************
*
*  SimpleDelayExit
*
*  Minimal child process: prints "started", delays briefly (10ms),
*  prints "quitting", and exits with code -3. Used as a lightweight
*  child in tests that need a process to exist temporarily.
*
*  Parameters:
*    pArgs - process name string (cast to char*), may be NULL
*
*********************************************************************************/
int SimpleDelayExit(void* pArgs)
{
    if (pArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", (char*)pArgs);
        SystemDelay(10);
        console_output(FALSE, "%s: quitting\n", (char*)pArgs);
    }

    k_exit(-3);

    return 0;
}

/*********************************************************************************
*
*  SimpleBlockExit
*
*  Child process that blocks on status code 14 until externally unblocked.
*  Prints "started" before blocking and "quitting" after being unblocked.
*  Exits with code -3.
*
*  Parameters:
*    pArgs - process name string (cast to char*), may be NULL
*
*********************************************************************************/
int SimpleBockExit(void* pArgs)
{
    if (pArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", (char*)pArgs);
        block(14);
        console_output(FALSE, "%s: quitting\n", (char*)pArgs);
    }

    k_exit(-3);

    return 0;
}

/*********************************************************************************
*
*  SpawnTwoPriorityFour
*
*  Spawns two SimpleDelayExit children at priority 4, then waits for both
*  to complete. Reports the exit status of each child. Exits with code -3.
*
*  Parameters:
*    strArgs - process name string
*
*********************************************************************************/
int SpawnTwoPriorityFour(char* strArgs)
{
    int kidpid;
    int status = 0xff;
    char nameBuffer[1024];

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);
        console_output(FALSE, "%s: performing spawn of first child\n", strArgs);

        /* Use the -Child naming convention for the child process name. */
        snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", strArgs);
        kidpid = k_spawn(nameBuffer, SimpleDelayExit, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
        console_output(FALSE, "%s: spawn of first child returned pid = %d\n", strArgs, kidpid);

        snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child2", strArgs);
        console_output(FALSE, "%s: performing spawn of second child\n", strArgs);
        kidpid = k_spawn(nameBuffer, SimpleDelayExit, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
        console_output(FALSE, "%s: spawn of second child returned pid = %d\n", strArgs, kidpid);

        console_output(FALSE, "%s: performing first wait.\n", strArgs);
        kidpid = k_wait(&status);
        console_output(FALSE, "%s: exit status for child %d is %d\n", strArgs,
            kidpid, status);

        console_output(FALSE, "%s: performing second wait.\n", strArgs);
        kidpid = k_wait(&status);
        console_output(FALSE, "%s: exit status for child %d is %d\n", strArgs,
            kidpid, status);
    }
    k_exit(-3);

    return 0;
}

/*********************************************************************************
*
*  SpawnTwoPriorityTwo
*
*  Spawns two SimpleDelayExit children at priority 2, then waits for both
*  to complete. Reports the exit status of each child. Exits with code 1.
*
*  Parameters:
*    strArgs - process name string
*
*********************************************************************************/
int SpawnTwoPriorityTwo(char* strArgs)
{
    int kidpid;
    int status = 0xff;
    char nameBuffer[1024];

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);
        console_output(FALSE, "%s: performing spawn of first child\n", strArgs);

        snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", strArgs);
        kidpid = k_spawn(nameBuffer, SimpleDelayExit, nameBuffer, THREADS_MIN_STACK_SIZE, 2);
        console_output(FALSE, "%s: spawn of first child returned pid = %d\n", strArgs, kidpid);

        snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child2", strArgs);
        console_output(FALSE, "%s: performing spawn of second child\n", strArgs);
        kidpid = k_spawn(nameBuffer, SimpleDelayExit, nameBuffer, THREADS_MIN_STACK_SIZE, 2);
        console_output(FALSE, "%s: spawn of second child returned pid = %d\n", strArgs, kidpid);

        console_output(FALSE, "%s: performing first wait\n", strArgs);
        kidpid = k_wait(&status);
        console_output(FALSE, "%s: exit status for child %d is %d\n", strArgs,
            kidpid, status);

        console_output(FALSE, "%s: performing second wait\n", strArgs);
        kidpid = k_wait(&status);
        console_output(FALSE, "%s: exit status for child %d is %d\n", strArgs,
            kidpid, status);
    }
    k_exit(1);

    return 0;
}

/*********************************************************************************
*
*  SpawnOnePriorityFour
*
*  Spawns one SimpleDelayExit child at priority 4, waits for it, and
*  reports its exit status. Exits with code -3.
*
*  Parameters:
*    strArgs - process name string
*
*********************************************************************************/
int SpawnOnePriorityFour(char* strArgs)
{
    int kidpid;
    int status = 0xff;
    char nameBuffer[1024];

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);
        console_output(FALSE, "%s: performing spawn of child\n", strArgs);

        snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", strArgs);
        kidpid = k_spawn(nameBuffer, SimpleDelayExit, nameBuffer, THREADS_MIN_STACK_SIZE, 4);
        console_output(FALSE, "%s: spawn of child returned pid = %d\n", strArgs, kidpid);

        kidpid = k_wait(&status);
        console_output(FALSE, "%s: exit status for child %d is %d\n", strArgs,
            kidpid, status);
    }
    k_exit(-3);

    return 0;
}

/*********************************************************************************
*
*  SpawnOnePriorityOne
*
*  Spawns one SimpleDelayExit child at priority 1 (highest), waits for it,
*  and reports its exit status. If the wait returns -1, reports that the
*  process was signaled. Exits with code -3.
*
*  Parameters:
*    strArgs - process name string
*
*********************************************************************************/
int SpawnOnePriorityOne(char* strArgs)
{
    int kidpid;
    int status = 0xff;
    char testName[1024];

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);
        console_output(FALSE, "%s: performing spawn of child\n", strArgs);

        snprintf(testName, sizeof(testName), "%s-Child1", strArgs);
        kidpid = k_spawn(testName, SimpleDelayExit, testName, THREADS_MIN_STACK_SIZE, 1);
        console_output(FALSE, "%s: spawn of child returned pid = %d\n", strArgs, kidpid);

        kidpid = k_wait(&status);
        if (kidpid == -1)
        {
            console_output(FALSE, "%s: process was signaled  during wait()\n", testName);
        }
        else
        {
            console_output(FALSE, "%s: exit status for child %d is %d\n", testName, kidpid, status);
        }
    }
    k_exit(-3);

    return 0;
}

/*********************************************************************************
*
*  SignalAndJoinTwoLower
*
*  Spawns two SpawnOnePriorityOne children at priority 2. After they start
*  (and each spawn their own priority-1 grandchild), this process:
*    1) Signals (SIG_TERM) and joins the first child.
*    2) Signals (SIG_TERM) and joins the second child.
*    3) Waits for any remaining grandchildren.
*  Calls display_process_table() between each step for debugging.
*  Exits with code -3.
*
*  Parameters:
*    strArgs - process name string
*
*********************************************************************************/
int SignalAndJoinTwoLower(char* strArgs)
{
    int child1, child2;
    int status = 0xff;
    char nameBuffer[1024];
    int exitCode;

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);

        console_output(FALSE, "%s: performing spawn of first child\n", strArgs);
        snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", strArgs);
        child1 = k_spawn(nameBuffer, SpawnOnePriorityOne, nameBuffer, THREADS_MIN_STACK_SIZE, 2);
        console_output(FALSE, "%s: spawn of first child returned pid = %d\n", strArgs, child1);

        console_output(FALSE, "%s: performing spawn of second child\n", strArgs);
        snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child2", strArgs);
        child2 = k_spawn(nameBuffer, SpawnOnePriorityOne, nameBuffer, THREADS_MIN_STACK_SIZE, 2);
        console_output(FALSE, "%s: spawn of first child returned pid = %d\n", strArgs, child2);

        display_process_table();

        console_output(FALSE, "%s: signaling first child\n", strArgs);
        k_kill(child1, SIG_TERM);
        k_join(child1, &exitCode);
        console_output(FALSE, "%s: after joining first child, status = %d\n", strArgs, exitCode);

        display_process_table();

        console_output(FALSE, "%s: signaling second child\n", strArgs);
        k_kill(child2, SIG_TERM);
        k_join(child2, &exitCode);
        console_output(FALSE, "%s: after joining second child, status = %d\n", strArgs, exitCode);

        display_process_table();

        console_output(FALSE, "%s: performing joins\n", strArgs);
        child1 = k_wait(&status);
        console_output(FALSE, "%s: exit status for child %d is %d\n", strArgs,
            child1, status);

        child2 = k_wait(&status);
        console_output(FALSE, "%s: exit status for child %d is %d\n", strArgs,
            child2, status);
    }
    k_exit(-3);
    return 0;
}


/*********************************************************************************
*
*  GetChildNumber
*
*  Extracts the trailing integer from a process name string. Used to
*  determine which child number a process is (e.g. "Test-Child3" returns 3).
*
*  Parameters:
*    name - process name string ending in digits
*
*  Returns:
*    The integer value of the trailing digits, or 0 if none found.
*
*********************************************************************************/
int GetChildNumber(char* name)
{
    int intIndex;
    int testNumber = 0;

    // Simple extraction of the test number
    intIndex = (int)strlen(name) - 1;
    while (intIndex >= 0 && isdigit(name[intIndex]))
    {
        intIndex--;
    }
    testNumber = atoi(&name[intIndex + 1]);

    return testNumber;
}

int pidToJoin;
int pids[BLOCK_UNBLOCK_COUNT];

/*********************************************************************************
*
*  UnblockTwoPriorities
*
*  Iterates through the global pids[] array and unblocks each process.
*  Unblocks BLOCK_UNBLOCK_COUNT * 2 processes total. Exits with code -2.
*
*  Parameters:
*    strArgs - process name string
*
*********************************************************************************/
int UnblockTwoPriorities(char* strArgs)
{
    int status = 0;

    console_output(FALSE, "%s: started\n", strArgs);

    for (int i = 0; i < BLOCK_UNBLOCK_COUNT * 2; ++i)
    {
        console_output(FALSE, "%s: Unblocking process %d\n", strArgs, pids[i]);
        unblock(pids[i]);
    }

    k_exit(-2);

    return 0;
}

/*********************************************************************************
*
*  SpawnJoiner
*
*  Joins (k_join) the process identified by the global pidToJoin variable.
*  Reports the join result and exit status. Exits with code -2.
*
*  Parameters:
*    strArgs - process name string
*
*********************************************************************************/
int SpawnJoiner(char* strArgs)
{
    int status = 0xff;
    int result;

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);
        console_output(FALSE, "%s: joining target\n", strArgs);
        result = k_join(pidToJoin, &status);
        console_output(FALSE, "%s: k_join for pid %d returned %d with status %d\n", strArgs, pidToJoin, result, status);

    }
    k_exit(-2);

    return 0;

}

/*********************************************************************************
*
*  DumpProcess
*
*  Simple child process that calls display_process_table() and exits.
*  Used to capture a snapshot of the process table at a specific point
*  in a test scenario. Exits with code -3.
*
*  Parameters:
*    strArgs - process name string
*
*********************************************************************************/
int DumpProcess(char* strArgs)
{
    int status = 0xff;

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);
        display_process_table();
    }
    k_exit(-3);

    return 0;
}

/*********************************************************************************
*
*  SpawnDumpProcess
*
*  Spawns a DumpProcess child at priority 2 and waits for it. Sets the
*  global gPid to the child's pid (if gPid has not already been set).
*  Reports the child's exit status, or notes if signaled. Exits with code -3.
*
*  Parameters:
*    strArgs - process name string
*
*********************************************************************************/
int SpawnDumpProcess(char* strArgs)
{
    int kidpid;
    int status = 0xff;
    char nameBuffer[1024];

    if (strArgs != NULL)
    {
        console_output(FALSE, "%s: started\n", strArgs);
        console_output(FALSE, "%s: performing spawn of child\n", strArgs);

        snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", strArgs);
        kidpid = k_spawn(nameBuffer, DumpProcess, nameBuffer, THREADS_MIN_STACK_SIZE, 2);
        console_output(FALSE, "%s: spawn of child returned pid = %d\n", strArgs, kidpid);
        /* If the global pid is not set, then set it to kidpid. */
        if (gPid == -1)
        {
            gPid = kidpid;
        }

        kidpid = k_wait(&status);
        if (kidpid == -5)
        {
            console_output(FALSE, "%s: process was signaled during wait\n", strArgs);
        }
        else
        {
            console_output(FALSE, "%s: exit status for child %d is %d\n", strArgs,
                kidpid, status);
        }
    }
    k_exit(-3);

    return 0;
}


/*********************************************************************************
*
*  SignalJoinGlobalPid
*
*  Sends SIG_TERM to the process identified by the global gPid, then
*  joins it and reports the exit status. Exits with code 5.
*
*  Parameters:
*    arg - process name string
*
*********************************************************************************/
int SignalJoinGlobalPid(char* arg)
{
    int exitCode;

    if (arg != NULL)
    {
        console_output(FALSE, "%s: started\n", arg);
        console_output(FALSE, "%s: signaling %d.\n", arg, gPid);
        k_kill(gPid, SIG_TERM);
        console_output(FALSE, "%s: joining %d.\n", arg, gPid);
        k_join(gPid, &exitCode);
        console_output(FALSE, "%s: after joining child, pid %d, status = %d\n", arg, gPid, exitCode);
    }

    k_exit(5);
    return 0;
}

/*********************************************************************************
*
*  JoinProcess
*
*  Joins the process identified by the global gPid (without signaling it
*  first) and reports the exit status. Exits with code 5.
*
*  Parameters:
*    arg - process name string
*
*********************************************************************************/
int JoinProcess(char* arg)
{
    int exitCode;

    if (arg != NULL)
    {
        console_output(FALSE, "%s: started\n", arg);
        console_output(FALSE, "%s: joining %d\n", arg, gPid);
        k_join(gPid, &exitCode);
        console_output(FALSE, "%s: after joining child, pid %d, status = %d\n", arg, gPid, exitCode);
    }

    k_exit(5);
    return 0;
}
