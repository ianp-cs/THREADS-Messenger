/* ------------------------------------------------------------------------
   Messaging.c
   College of Applied Science and Technology
   The University of Arizona
   CYBV 489

   Student Names:  Ian Penrose and Lindsay Wax

   ------------------------------------------------------------------------ */
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <THREADSLib.h>
#include <Scheduler.h>
#include <Messaging.h>
#include <stdint.h>
#include "message.h"


#define DEVICE_NAME_LEN 16

/* ------------------------- Prototypes ----------------------------------- */
static void nullsys(system_call_arguments_t* args);

/* Note: interrupt_handler_t is already defined in THREADSLib.h with the signature:
 *   void (*)(char deviceId[32], uint8_t command, uint32_t status, void *pArgs)
 */

static void InitializeHandlers();
static int check_io_messaging(void);
extern int MessagingEntryPoint(void*);
static void checkKernelMode(const char* functionName);

struct psr_bits {
    unsigned int cur_int_enable : 1;
    unsigned int cur_mode : 1;
    unsigned int prev_int_enable : 1;
    unsigned int prev_mode : 1;
    unsigned int unused : 28;
};

union psr_values {
    struct psr_bits bits;
    unsigned int integer_part;
};


// New Functions
static void initializeAllDevices();
static int findMbox(int mboxId);
static void cleanMailbox(int index);

/* -------------------------- Globals ------------------------------------- */

/* Obtained from THREADS*/
interrupt_handler_t* handlers;

/* system call array of function pointers */
void (*systemCallVector[THREADS_MAX_SYSCALLS])(system_call_arguments_t* args);

/* the mail boxes */
MailBox mailboxes[MAXMBOX];
MailSlot mailSlots[MAXSLOTS];

/* waiting process table */
WaitingProcess waitingProcsTable[MAX_PROCESSES];

typedef struct
{
    void* deviceHandle;
    int deviceMbox;
    int deviceType;
    char deviceName[DEVICE_NAME_LEN];
} DeviceManagementData;

static DeviceManagementData devices[THREADS_MAX_DEVICES];
static int nextMailboxId = 0;
static int waitingOnDevice = 0;


/* ------------------------------------------------------------------------
     Name - SchedulerEntryPoint
     Purpose - Initializes mailboxes and interrupt vector.
               Start the Messaging test process.
     Parameters - one, default arg passed by k_spawn that is not used here.
----------------------------------------------------------------------- */
int SchedulerEntryPoint(void* arg)
{
    checkKernelMode("SchedulerEntryPoint");

    /* Disable interrupts */
    disableInterrupts();

    /* set this to the real check_io function. */
    check_io = check_io_messaging;

    /* Initialize the mail box table, slots, & other data structures.
     * Initialize int_vec and sys_vec, allocate mailboxes for interrupt
     * handlers.  Etc... */

    for (int i = 0; i < MAXMBOX; i++)
    {
        mailboxes[i].pSlotListHead = NULL;
        mailboxes[i].mbox_id = -1;
        mailboxes[i].waitingProcsHead = NULL;
    }

    for (int i = 0; i < MAXSLOTS; i++)
    {
        mailSlots[i].pNextSlot = NULL;
        mailSlots[i].pPrevSlot = NULL;
        mailSlots[i].mbox_id = -1;
    }

    

    /* Initialize the devices and their mailboxes. */
    /* Allocate mailboxes for use by the interrupt handlers.
     * Note: The clock device uses a zero-slot mailbox, while I/O devices
     * (disks, terminals) need slotted mailboxes since their interrupt
     * handlers use non-blocking sends.
     */
    // TODO: Create mailboxes for each device.
    //   devices[THREADS_CLOCK_DEVICE_ID].deviceMbox = mailbox_create(0, sizeof(int));
    //   devices[i].deviceMbox = mailbox_create(..., sizeof(int));
    devices[THREADS_CLOCK_DEVICE_ID].deviceMbox = mailbox_create(0, sizeof(int));

    for (int i = 0; i < THREADS_MAX_DEVICES; i++)
    {
        devices[i].deviceMbox = mailbox_create(MAXSLOTS, sizeof(int));
    }

    /* TODO: Initialize the devices using device_initialize().
     * The devices are: disk0, disk1, term0, term1, term2, term3.
     * Store the device handle and name in the devices array.
     */

    initializeAllDevices();

    InitializeHandlers();

    enableInterrupts();

    /* TODO: Create a process for Messaging, then block on a wait until messaging exits.*/
    k_spawn("Messaging", MessagingEntryPoint, NULL, 4 * THREADS_MIN_STACK_SIZE, HIGHEST_PRIORITY);

    int status = -1;
    k_wait(&status);

    k_exit(0);

    return 0;
} /* SchedulerEntryPoint */


/* ------------------------------------------------------------------------
   Name - mailbox_create
   Purpose - gets a free mailbox from the table of mailboxes and initializes it
   Parameters - maximum number of slots in the mailbox and the max size of a msg
                sent to the mailbox.
   Returns - -1 to indicate that no mailbox was created, or a value >= 0 as the
             mailbox id.
   ----------------------------------------------------------------------- */
int mailbox_create(int slots, int slot_size)
{
    checkKernelMode("mailbox_create");

    int newId = -1;

    // Input validation
    if (slots < 0 || slots > MAXSLOTS)
    {
        console_output(false, "mailbox_create(): Attempted to create a mailbox with an invalid number of slots.\n");
        return -1;
    }

    if (slot_size < 0 || slot_size > 256)
    {
        console_output(false, "mailbox_create(): Attempted to create a mailbox with an invalid slot size.\n");
        return -1;
    }

    // Find unused mailbox and initialize it
    for (int i = 0; i < MAXMBOX; i++)
    {
        if (mailboxes[i].mbox_id == -1)
        {
            newId = i;
            mailboxes[i].mbox_id = newId;
            mailboxes[i].slotCount = slots;
            mailboxes[i].slotSize = slot_size;
            mailboxes[i].status = MBSTATUS_EMPTY;

            if (slots == 0)
            {
                mailboxes[i].type = MB_ZEROSLOT;
            }
            else if (slots == 1)
            {
                mailboxes[i].type = MB_MULTISLOT;
            }
        }
    }

    if (newId == -1)
    {
        console_output(false, "mailbox_create(): Failed to create mailbox, there is no room for more mailboxes.\n");
    }

    return newId;
} /* mailbox_create */


/* ------------------------------------------------------------------------
   Name - mailbox_send
   Purpose - Put a message into a slot for the indicated mailbox.
             Block the sending process if no slot available.
   Parameters - mailbox id, pointer to data of msg, # of bytes in msg,
                block flag.
   Returns - zero if successful, -1 if invalid args, -2 if would block
             (non-blocking mode), -5 if signaled while waiting.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int mailbox_send(int mboxId, void* pMsg, int msg_size, int wait)
{
    checkKernelMode("mailbox_send");

    int result = -1;



    return result;
}

/* ------------------------------------------------------------------------
   Name - mailbox_receive
   Purpose - Receive a message from the indicated mailbox.
             Block the receiving process if no message available.
   Parameters - mailbox id, pointer to buffer for msg, max size of buffer,
                block flag.
   Returns - size of received msg (>=0) if successful, -1 if invalid args,
             -2 if would block (non-blocking mode), -5 if signaled.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int mailbox_receive(int mboxId, void* pMsg, int msg_size, int wait)
{
    checkKernelMode("mailbox_receive");

    int result = -1;

    return result;
}

/* ------------------------------------------------------------------------
   Name - mailbox_free
   Purpose - Frees a previously created mailbox. Any process waiting on
             the mailbox should be signaled and unblocked.
   Parameters - mailbox id.
   Returns - zero if successful, -1 if invalid args, -5 if signaled
             while closing the mailbox.
   ----------------------------------------------------------------------- */
int mailbox_free(int mboxId)
{
    checkKernelMode("mailbox_free");

    int index = findMbox(mboxId);

    // No mbox matches ID
    if (index == -1)
    {
        return -1;
    }

    WaitingProcess* proc = mailboxes[index].waitingProcsHead;

    while (proc != NULL)
    {
        // Tell waiting process to terminate and unblock it
        k_kill(proc->pid, SIG_TERM);
        unblock(proc->pid);

        // Clean up the process in the process table
        WaitingProcess* nextProc = proc->pNextProcess;

        proc->pid = -1;
        proc->pNextProcess = NULL;
        proc->pPrevProcess = NULL;
        proc->index = -1;

        proc = nextProc;
    }

    cleanMailbox(index);

    // Process signaled while waiting closing mbox
    if (signaled())
    {
        return -5;
    }

    return 0;
}

/* ------------------------------------------------------------------------
   Name - wait_device
   Purpose - Waits for a device interrupt by blocking on the device's
             mailbox. Returns the device status via the status pointer.
   Parameters - device name string, pointer to status output.
   Returns - 0 if successful, -1 if invalid parameter, -5 if signaled.
   ----------------------------------------------------------------------- */
int wait_device(char* deviceName, int* status)
{
    int result = 0;
    uint32_t deviceHandle = -1;
    checkKernelMode("waitdevice");

    enableInterrupts();

    if (strcmp(deviceName, "clock") == 0)
    {
        deviceHandle = THREADS_CLOCK_DEVICE_ID;
    }
    else
    {
        deviceHandle = device_handle(deviceName);

    }

    if (deviceHandle >= 0 && deviceHandle < THREADS_MAX_DEVICES)
    {
        /* set a flag that there is a process waiting on a device. */
        waitingOnDevice++;
        mailbox_receive(devices[deviceHandle].deviceMbox, status, sizeof(int), TRUE);

        disableInterrupts();

        waitingOnDevice--;
    }
    else
    {
        console_output(FALSE, "Unknown device type.");
        stop(-1);
    }

    /* spec says return -5 if signaled. */
    if (signaled())
    {
        result = -5;
    }

    return result;
}


int check_io_messaging(void)
{
    if (waitingOnDevice)
    {
        return 1;
    }
    return 0;
}

static void InitializeHandlers()
{
    handlers = get_interrupt_handlers();

    /* TODO: Register interrupt handlers in the handlers array.
     * Use the interrupt indices defined in THREADSLib.h:
     *   handlers[THREADS_TIMER_INTERRUPT]   = your_clock_handler;
     *   handlers[THREADS_IO_INTERRUPT]      = your_io_handler;
     *   handlers[THREADS_SYS_CALL_INTERRUPT] = your_syscall_handler;
     *
     * Also initialize the system call vector (systemCallVector).
     */

}

/* an error method to handle invalid syscalls */
static void nullsys(system_call_arguments_t* args)
{
    console_output(FALSE,"nullsys(): Invalid syscall %d. Halting...\n", args->call_id);
    stop(1);
} /* nullsys */


/*****************************************************************************
   Name - checkKernelMode
   Purpose - Checks the PSR for kernel mode and halts if in user mode
   Parameters -
   Returns -
****************************************************************************/
static inline void checkKernelMode(const char* functionName)
{
    union psr_values psrValue;

    psrValue.integer_part = get_psr();
    if (psrValue.bits.cur_mode == 0)
    {
        console_output(FALSE, "Kernel mode expected, but function called in user mode.\n");
        stop(1);
    }
}


static void initializeAllDevices()
{
    checkKernelMode("initializeAllDevices");

    char* deviceNames[THREADS_MAX_DEVICES - 1] = 
        {"clock", "disk0", "disk1", "term0", "term1", "term2", "term3" };

    for (int i = 0; i < THREADS_MAX_DEVICES - 1; i++)
    {
        strncpy_s(devices[i].deviceName, DEVICE_NAME_LEN, deviceNames[i], DEVICE_NAME_LEN);
        devices[i].deviceHandle = device_initialize(devices[i].deviceName);

        if (devices[i].deviceHandle == -1)
        {
            console_output(false, "Device %s failed to initialize.\n\n", deviceNames[i]);
        }
    }
}


static int findMbox(int mboxId)
{
    for (int i = 0; i < MAXMBOX; i++)
    {
        if (mailboxes[i].mbox_id == mboxId)
        {
            return i;
        }
    }

    return -1;
}


static void cleanMailbox(int index)
{
    MailSlot* slot = mailboxes[index].pSlotListHead;

    // Clean out every slot in the mailbox
    while (slot != NULL)
    {
        MailSlot* nextSlot = slot->pNextSlot;

        slot->pNextSlot = NULL;
        slot->pPrevSlot = NULL;
        slot->mbox_id = -1;
        strcpy(slot->message, "");
        slot->messageSize = 0;

        slot = nextSlot;
    }

    mailboxes[index].pSlotListHead = NULL;
    mailboxes[index].mbox_id = -1;
    mailboxes[index].waitingProcsHead = NULL;
}
