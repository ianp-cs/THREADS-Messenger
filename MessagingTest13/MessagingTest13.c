#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"

int WaitForClockThenExit(char* strArgs);

/*********************************************************************************
*
* MessagingTest13 - Clock Device wait_device
*
* Spawns a single child process that calls wait_device("clock", ...) to block
* until the clock interrupt handler delivers a message to the clock device
* mailbox. The clock handler fires every 5 ticks (~100ms), sending a
* zero-value message to the clock mailbox.
*
* Tests the integration between the clock interrupt handler, the device
* mailbox system, and wait_device. Verifies that a process can successfully
* block on a device mailbox and be woken by an interrupt.
*
* Expected: Child blocks on wait_device, wakes after clock interrupt,
*           prints status, and exits with -3.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
    char* testName = GetTestName(__FILE__);
    int status, kidpid;
    char nameBuffer[512];

    console_output(FALSE, "\n%s: started\n", testName);

    /* Use the -Child naming convention for the child process name. */
    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1", testName);

    kidpid = k_spawn(nameBuffer, WaitForClockThenExit, nameBuffer, THREADS_MIN_STACK_SIZE, 4);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, nameBuffer, status);

    k_exit(0);

    return 0;
} /* MessagingEntryPoint */

int WaitForClockThenExit(char* strArgs)
{
    int i, result;
    char buffer[20];
    int status;

    console_output(FALSE, "%s: started\n", strArgs);

    /* wait for the device to respond. */
    wait_device("clock", &status);

    console_output(FALSE, "\n%s: Returned from waitdevice, status: 0x%08x.\n", strArgs, status);

    k_exit(-3);

    return 0;
} /* SendSixAndExit */
