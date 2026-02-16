#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "THREADSLib.h"
#include "Scheduler.h"
#include "Messaging.h"
#include "TestCommon.h"
int TerminalReceiveTesting(char* strArgs);
int mailboxId;

/*********************************************************************************
*
* MessagingTest32 - Multiple Terminal Device I/O
*
* Writes a character to each of the four terminals (term0-term3) using
* device_control with TERMINAL_WRITE_CHAR ('A' through 'D'). Then spawns
* four child processes, each calling wait_device on a different terminal
* to receive the I/O completion interrupt.
*
* Each child parses its terminal number from the process name (appended
* after a ':' separator), constructs the device name "termN", and blocks
* on wait_device until the terminal interrupt handler delivers the status.
*
* Tests concurrent device I/O across all four terminals and that each
* terminal's device mailbox correctly delivers to the correct waiter.
*
* Expected: All four children receive their respective terminal status
*           and exit with -3.
*
*********************************************************************************/
int MessagingEntryPoint(void* pArgs)
{
    char* testName = GetTestName(__FILE__);
    int status, kidpid;
    char nameBuffer[512];
    device_control_block_t controlBlock;
    char childNames[MAXPROC][256];

    console_output(FALSE, "\n%s: started\n", testName);

    /* send a character to the terminal. */
    controlBlock.command = TERMINAL_WRITE_CHAR;
    controlBlock.output_data = (void*)'A';
    device_control("term0", controlBlock);

    /* send a character to the terminal. */
    controlBlock.command = TERMINAL_WRITE_CHAR;
    controlBlock.output_data = (void*)'B';
    device_control("term1", controlBlock);

    /* send a character to the terminal. */
    controlBlock.command = TERMINAL_WRITE_CHAR;
    controlBlock.output_data = (void*)'C';
    device_control("term2", controlBlock);

    /* send a character to the terminal. */
    controlBlock.command = TERMINAL_WRITE_CHAR;
    controlBlock.output_data = (void*)'D';
    device_control("term3", controlBlock);


    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child1:0", testName);
    kidpid = k_spawn(nameBuffer, TerminalReceiveTesting, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    nameBuffer[strlen(nameBuffer) - 2] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child2:1", testName);
    kidpid = k_spawn(nameBuffer, TerminalReceiveTesting, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    nameBuffer[strlen(nameBuffer) - 2] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child3:2", testName);
    kidpid = k_spawn(nameBuffer, TerminalReceiveTesting, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    nameBuffer[strlen(nameBuffer) - 2] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);

    snprintf(nameBuffer, sizeof(nameBuffer), "%s-Child4:3", testName);
    kidpid = k_spawn(nameBuffer, TerminalReceiveTesting, nameBuffer, THREADS_MIN_STACK_SIZE, 3);
    nameBuffer[strlen(nameBuffer) - 2] = '\0';
    strncpy(childNames[kidpid], nameBuffer, 256);


    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    kidpid = k_wait(&status);
    console_output(FALSE, "%s: Exit status for child %s is %d\n", testName, childNames[kidpid], status);

    k_exit(0);

    return 0;
}

int TerminalReceiveTesting(char* strArgs)
{
    int result;
    char* separator;
    int termToRead;
    char terminalDeviceName[16];
    int status;

    if (strArgs != NULL)
    {
        separator = strchr(strArgs, ':');
        separator[0] = '\0';
        separator++;
        sscanf(separator, "%d", &termToRead);

        console_output(FALSE, "%s: started\n", strArgs);

        sprintf(terminalDeviceName, "term%d", termToRead);

        result = wait_device(terminalDeviceName, &status);
        console_output(FALSE, "%s: wait_device returned %d with status 0x%08x\n", strArgs, result, status);

    }

    k_exit(-3);

    return 0;
}
