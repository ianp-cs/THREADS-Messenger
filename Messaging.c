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
#define GLOBAL_SLOT_POOL_SIZE MAXSLOTS * 10

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
static void clock_handler_messaging(char* device, uint8_t command, uint32_t status, void* pArgs);
static void io_handler_messaging(char* device, uint8_t command, uint32_t status, void* pArgs);
static void sys_call_handler_messaging(char* device, uint8_t command, uint32_t status, void* pArgs);
static MailSlot* findEmptySlot(int index);
static void addWaitingProc(int index, int code, void* pMsg, int msg_size);
static void removeWaitingProc(int index, int targetPid);
static void createMailSlots(int index, int slotCount);
static int findUnusedSlot();
static void clearMailSlot(MailSlot* target, int index);
static void unblockWaitingProc(int mbox_id, int code, MailSlot* slot);
static void reorderMailSlots(int index);
static WaitingProcessPtr addFreeProc(int index);
static MailSlot* findMail(int index);


/* -------------------------- Globals ------------------------------------- */

/* Obtained from THREADS*/
interrupt_handler_t* handlers;

/* system call array of function pointers */
void (*systemCallVector[THREADS_MAX_SYSCALLS])(system_call_arguments_t* args);

/* the mail boxes */
MailBox mailboxes[MAXMBOX];
MailSlot mailSlots[GLOBAL_SLOT_POOL_SIZE];

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
        mailboxes[i].fullSlots = 0;
        mailboxes[i].slotCount = 0;
        mailboxes[i].mbox_id = -1;
        mailboxes[i].waitingProcsHead = NULL;
        mailboxes[i].waitingProcsCount = 0;
        mailboxes[i].freeProc = NULL;
    }

    for (int i = 0; i < GLOBAL_SLOT_POOL_SIZE; i++)
    {
        mailSlots[i].pNextSlot = NULL;
        mailSlots[i].pPrevSlot = NULL;
        mailSlots[i].mbox_id = -1;
        mailSlots[i].inUse = 0;
        mailSlots[i].senderPid = 0;
        mailSlots[i].receiverPid = 0;
        mailSlots[i].messageSize = 0;
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

    for (int i = 1; i < THREADS_MAX_DEVICES; i++)
    {
        devices[i].deviceMbox = mailbox_create(5, sizeof(int));
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
            createMailSlots(i, slots);

            if (slots == 0)
            {
                mailboxes[i].type = MB_ZEROSLOT;
            }
            else if (slots == 1)
            {
                mailboxes[i].type = MB_MULTISLOT;
            }

            break;
        }
    }

    /*
    if (newId == -1)
    {
        console_output(true, "mailbox_create(): Failed to create mailbox, there is no room for more mailboxes.\n");
    }
    */

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

    int activeProc = k_getpid(); // For testing
    int index = findMbox(mboxId);

    // Input validation

    if (index == -1)
    {
        console_output(false, "mailbox_send(): Mailbox ID is invalid.\n");
        return -1;
    }

    if (msg_size > mailboxes[index].slotSize || msg_size < 0)
    {
        console_output(false, "mailbox_send(): Message size is invalid.\n");
        return -1;
    }

    if (wait != 0 && wait != 1)
    {
        console_output(false, "mailbox_send(): Invalid block flag.\n");
        return -1;
    }

    if (mailboxes[index].fullSlots == MAXSLOTS)
    {
        console_output(false, "No mail slots available.\n");
        stop(1);
    }

    if (mailboxes[index].freeProc != NULL)
    {
        return -1;
    }

    // Mailbox is a zero slot mailbox
    if (mailboxes[index].slotCount == 0)
    {
        if (wait == 0 && mailboxes[index].waitingProcsHead == NULL)
        {
            return -2;
        }
        else if (wait == 0 && mailboxes[index].waitingProcsHead != NULL)
        {
            unblockWaitingProc(mailboxes[index].mbox_id, BLOCKED_RECEIVE, NULL);
        }
        else if (mailboxes[index].waitingProcsHead == NULL ||
            mailboxes[index].waitingProcsHead->blockedCode != BLOCKED_RECEIVE)
        {
            addWaitingProc(index, BLOCKED_SEND, pMsg, msg_size);
            block(BLOCKED_SEND);
            removeWaitingProc(index, k_getpid());
        }
        else
        {
            memcpy(mailboxes[index].waitingProcsHead->message, pMsg, msg_size);
            mailboxes[index].waitingProcsHead->messageSize = msg_size;
            unblockWaitingProc(mailboxes[index].mbox_id, BLOCKED_RECEIVE, NULL);
        }
    }
    // The mailbox has slots
    else
    {
        // Allocate mail slot
        MailSlot* target = findEmptySlot(index);

        if (target == NULL && wait == 0)
        {
            // console_output(false, "mailbox_send(): No available mail slot to store message.\n");
            return -2;
        }
        else if (target == NULL && wait == 1)
        {
            addWaitingProc(index, BLOCKED_SEND, NULL, 0);
            block(BLOCKED_SEND);
            target = findEmptySlot(index);
        }

        // Save message
        if (target != NULL && !signaled())
        {
            target->inUse = 1;
            target->messageSize = msg_size;
            target->senderPid = k_getpid();
            (mailboxes[index].fullSlots)++;
            memcpy(target->message, pMsg, msg_size);

            unblockWaitingProc(target->mbox_id, BLOCKED_RECEIVE, target);
        }
    }

    if (signaled())
    {
        // If last waiting proc, unblock the freeing process
        if (mailboxes[index].waitingProcsCount == 1)
        {
            unblock(mailboxes[index].freeProc->pid);
        }

        (mailboxes[index].waitingProcsCount)--;

        return -5;
    }

    return 0;
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

    int activeProc = k_getpid(); // For testing
    int index = findMbox(mboxId);
    int count = 0;

    // Input validation

    if (index == -1)
    {
        console_output(false, "mailbox_receive(): Mailbox ID is invalid.\n");
        return -1;
    }

    if (msg_size < 0)
    {
        console_output(false, "mailbox_receive(): Message size is invalid.\n");
        return -1;
    }

    if (wait != 0 && wait != 1)
    {
        console_output(false, "mailbox_receive(): Invalid block flag.\n");
        return -1;
    }

    if (mailboxes[index].freeProc != NULL)
    {
        return -1;
    }

    // This is a zero slot mailbox
    if (mailboxes[index].slotCount == 0)
    {
        if (wait == 0 && mailboxes[index].waitingProcsHead == NULL)
        {
            return -2;
        }
        else if (wait == 0 && mailboxes[index].waitingProcsHead != NULL)
        {
            unblockWaitingProc(mailboxes[index].mbox_id, BLOCKED_SEND, NULL);
        }
        else if (mailboxes[index].waitingProcsHead == NULL
            || mailboxes[index].waitingProcsHead->blockedCode != BLOCKED_SEND)
        {
            addWaitingProc(index, BLOCKED_RECEIVE, NULL, 0);
            block(BLOCKED_RECEIVE);
            memcpy(pMsg, mailboxes[index].waitingProcsHead->message, msg_size);

            // Determine actual length of message (there may not be a null terminator, so we can't use [strlen() + 1]
            for (int i = 0; i < msg_size; i++)
            {
                count++;
                if ((mailboxes[index].waitingProcsHead->message)[i] == '\0')
                {
                    break;
                }
            }

            removeWaitingProc(index, k_getpid());
        }
        else
        {
            memcpy(pMsg, mailboxes[index].waitingProcsHead->message, msg_size);

            // Determine actual length of message (there may not be a null terminator, so we can't use [strlen() + 1]
            for (int i = 0; i < msg_size; i++)
            {
                count++;
                if ((mailboxes[index].waitingProcsHead->message)[i] == '\0')
                {
                    break;
                }
            }

            unblockWaitingProc(mailboxes[index].mbox_id, BLOCKED_SEND, NULL);
        }
    }
    // The mailbox has slots
    else {
        // Check for mail. 
        // Mailbox is empty and blocking mode is off
        if (mailboxes[index].fullSlots == 0 && wait == 0)
        {
            return -2;
        }
        // Mailbox is empty and blocking mode is on
        else if (mailboxes[index].fullSlots == 0 && wait == 1)
        {
            addWaitingProc(index, BLOCKED_RECEIVE, NULL, 0);
            block(BLOCKED_RECEIVE);
        }

        // Mailbox has mail
        MailSlot* target = findMail(index);

        // Determine actual length of message (there may not be a null terminator, so we can't use [strlen() + 1]
        for (int i = 0; i < msg_size; i++)
        {
            count++;
            if ((target->message)[i] == '\0')
            {
                break;
            }
        }

        if (!signaled())
        {
            memcpy(pMsg, target->message, msg_size);
            clearMailSlot(target, index);
            reorderMailSlots(index);
        }
    }

    if (signaled())
    {
        // If last waiting proc, unblock the freeing process
        if (mailboxes[index].waitingProcsCount == 1)
        {
            unblock(mailboxes[index].freeProc->pid);
        }

        (mailboxes[index].waitingProcsCount)--;

        return -5;
    }

    return count;
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
    mailboxes[index].freeProc = addFreeProc(index);

    while (proc != NULL)
    {
        WaitingProcess* nextProc = proc->pNextProcess;

        // Tell waiting process to terminate and unblock it
        k_kill(proc->pid, SIG_TERM);
        unblock(proc->pid);

        proc->pid = 0;
        proc->index = -1;
        proc->blockedCode = 0;
        proc->messageSize = 0;
        strcpy_s(proc->message, proc->messageSize+1, "");
        proc->pNextProcess = NULL;
        proc->pPrevProcess = NULL;

        proc = nextProc;
    }

    if (mailboxes[index].waitingProcsCount != 0)
    {
        block(BLOCKED_RELEASE);
    }
    cleanMailbox(index);

    // Process signaled while closing mbox
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

    handlers[THREADS_TIMER_INTERRUPT] = clock_handler_messaging;
    handlers[THREADS_IO_INTERRUPT] = io_handler_messaging;
    handlers[THREADS_SYS_CALL_INTERRUPT] = sys_call_handler_messaging;

    // Also initialize the system call vector (systemCallVector).
    for (int i = 0; i < THREADS_MAX_SYSCALLS; i++)
    {
        systemCallVector[i] = nullsys;
    }
}

/* an error method to handle invalid syscalls */
static void nullsys(system_call_arguments_t* args)
{
    console_output(FALSE, "nullsys(): Invalid syscall %d. Halting...\n", args->call_id);
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
    { "clock", "disk0", "disk1", "term0", "term1", "term2", "term3" };

    for (int i = 0; i < THREADS_MAX_DEVICES - 1; i++)
    {
        strncpy_s(devices[i].deviceName, DEVICE_NAME_LEN, deviceNames[i], DEVICE_NAME_LEN);
        (uint32_t)devices[i].deviceHandle = device_initialize(devices[i].deviceName);

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
    WaitingProcessPtr proc = mailboxes[index].freeProc;

    // Clean out every slot in the mailbox
    while (slot != NULL)
    {
        MailSlot* nextSlot = slot->pNextSlot;

        slot->pNextSlot = NULL;
        slot->pPrevSlot = NULL;
        slot->mbox_id = -1;
        strcpy_s(slot->message, (slot->messageSize) + 1, "");
        slot->messageSize = 0;

        slot = nextSlot;
    }

    // Clean freeProc object
    proc->pid = 0;
    proc->index = -1;
    proc->blockedCode = 0;
    proc->pNextProcess = NULL;
    proc->pPrevProcess = NULL;

    // Clean mailbox object
    mailboxes[index].pSlotListHead = NULL;
    mailboxes[index].mbox_id = -1;
    mailboxes[index].waitingProcsHead = NULL;
    mailboxes[index].freeProc = NULL;
}

/**************************************************************************
   Name - clock_handler

   Purpose - This function is used by the interrupt handler to create time slices for processes
             and facilitate the round-robin with priority system for CPU time.

   Note - Not sure exactly what the parameters are used for, since a lot of the interrupt handler is
          hidden within libraries we can't access. Assume the parameters are needed, even if they
          aren't used within the function.
   *************************************************************************/
static void clock_handler_messaging(char* device, uint8_t command, uint32_t status, void* pArgs)
{
    static int count = 0;
    int clock_result = 0;
    int result = 0;
    uint32_t clockMbox;

    if (count == 4)
    {
        clockMbox = devices[THREADS_CLOCK_DEVICE_ID].deviceMbox;
        mailbox_send(clockMbox, NULL, 0, 0);
        count = 0;
    }
    else
    {
        count++;
    }

    time_slice();
}


static void io_handler_messaging(char* device, uint8_t command, uint32_t status, void* pArgs)
{
    LARGE_INTEGER unit;
    int result;
    int deviceId;

    unit.QuadPart = (LONGLONG)device;
    deviceId = unit.LowPart;

    if (deviceId >= THREADS_MAX_DEVICES)
    {
        console_output(false, "io_handler(): device id out of range.\n");
        return;
    }

    result = mailbox_send(devices[deviceId].deviceMbox, &status, sizeof(int), false);

    if (result == -1)
    {
        console_output(false, "io_handler(): invalid parameter passed to mailbox_receive(). Closing program.\n");
        stop(1);
    }
    else if (result == -5)
    {
        console_output(false, "io_handler(): process signaled while waiting. Closing program.\n");
        stop(1);
    }
}


static void sys_call_handler_messaging(char* device, uint8_t command, uint32_t status, void* pArgs)
{
    LARGE_INTEGER unit;
    int deviceId;

    unit.QuadPart = (LONGLONG)device;
    deviceId = unit.LowPart;

    if (deviceId >= THREADS_MAX_DEVICES)
    {
        console_output(false, "io_handler(): device id out of range.\n");
        return;
    }

    if (pArgs != NULL)
    {
        system_call_arguments_t* pSysCallArgs = pArgs;

        // if(pSysCallArgs->call_id == 7)

        if (systemCallVector[pSysCallArgs->call_id] != NULL)
        {
            systemCallVector[pSysCallArgs->call_id](pArgs);
        }
    }
}


static MailSlot* findEmptySlot(int index)
{
    if (signaled())
    {
        return NULL;
    }

    if (mailboxes[index].fullSlots == mailboxes[index].slotCount)
    {
        return NULL;
    }

    MailSlot* currSlot = mailboxes[index].pSlotListHead;

    while (currSlot != NULL)
    {
        if (currSlot->inUse == 0)
        {
            return currSlot;
        }

        currSlot = currSlot->pNextSlot;
    }

    console_output(false, "findEmptySlot(): fullSlots/slotCount mismatch.\n");

    return NULL;
}


static void addWaitingProc(int index, int code, void* pMsg, int msg_size)
{
    int target = -1;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (waitingProcsTable[i].pid == 0)
        {
            target = i;
            waitingProcsTable[target].pid = k_getpid();
            waitingProcsTable[target].index = target;
            waitingProcsTable[target].blockedCode = code;
            memcpy(waitingProcsTable[target].message, pMsg, msg_size);
            waitingProcsTable[target].messageSize = msg_size;
            break;
        }
    }

    if (target == -1)
    {
        console_output(false, "addWaitingProc(): Waiting procedures table is full. Closing program...\n");
        stop(1);
    }

    WaitingProcess* currProc = mailboxes[index].waitingProcsHead;

    if (currProc != NULL)
    {
        while (currProc->pNextProcess != NULL)
        {
            currProc = currProc->pNextProcess;
        }

        currProc->pNextProcess = &waitingProcsTable[target];
        currProc->pNextProcess->pPrevProcess = currProc;
    }
    else
    {
        mailboxes[index].waitingProcsHead = &waitingProcsTable[target];
    }

    (mailboxes[index].waitingProcsCount)++;
}


static void removeWaitingProc(int index, int targetPid)
{
    disableInterrupts();
    if (signaled())
    {
        return;
    }

    WaitingProcess* target = mailboxes[index].waitingProcsHead;

    if (target == NULL)
    {
        console_output(false, "removeWaitingProc(): Mailbox has no waiting processes\n");
        stop(1);
    }

    while (target->pid != targetPid)
    {
        target = target->pNextProcess;

        if (target == NULL)
        {
            target = mailboxes[index].waitingProcsHead;
            break;
            //console_output(false, "removeWaitingProc(): Couldn't find current process in the table. Closing program...\n");
            //stop(1);
        }
    }

    // Target is head of waiting processes list
    if (target->pPrevProcess == NULL)
    {
        if (target->pNextProcess == NULL)
        {
            mailboxes[index].waitingProcsHead = NULL;
        }
        else
        {
            mailboxes[index].waitingProcsHead = target->pNextProcess;
            target->pNextProcess->pPrevProcess = NULL;
        }
    }
    // Target is not the head of the waiting processes list
    else
    {
        target->pPrevProcess->pNextProcess = target->pNextProcess;

        if (target->pNextProcess != NULL)
        {
            target->pNextProcess->pPrevProcess = target->pPrevProcess;
        }
    }

    // Clean up target
    target->pid = 0;
    target->index = -1;
    target->blockedCode = 0;
    target->messageSize = 0;
    strcpy_s(target->message, sizeof(target->message), "");
    target->pNextProcess = NULL;
    target->pPrevProcess = NULL;

    (mailboxes[index].waitingProcsCount)--;
    enableInterrupts();

    return;
}


static void createMailSlots(int index, int slotCount)
{
    if (slotCount == 0)
    {
        return;
    }

    int slotIndex = findUnusedSlot();

    mailSlots[slotIndex].mbox_id = mailboxes[index].mbox_id;
    mailboxes[index].pSlotListHead = &mailSlots[slotIndex];
    MailSlot* prevSlot = &mailSlots[slotIndex];

    for (int i = 1; i < slotCount; i++)
    {
        slotIndex = findUnusedSlot();

        mailSlots[slotIndex].mbox_id = mailboxes[index].mbox_id;
        prevSlot->pNextSlot = &mailSlots[slotIndex];
        mailSlots[slotIndex].pPrevSlot = prevSlot;

        prevSlot = prevSlot->pNextSlot;
    }
}


static int findUnusedSlot()
{
    for (int i = 0; i < GLOBAL_SLOT_POOL_SIZE; i++)
    {
        if (mailSlots[i].mbox_id == -1)
        {
            return i;
        }
    }

    console_output(false, "findUnusedSlot(): No available mail slots in table.\n");
    return -1;
}


static void clearMailSlot(MailSlot* target, int index)
{
    strcpy_s(target->message, target->messageSize, "");
    target->messageSize = 0;
    target->inUse = 0;
    target->senderPid = 0;
    target->receiverPid = 0;
    (mailboxes[index].fullSlots)--;
}


static void unblockWaitingProc(int mbox_id, int code, MailSlot* slot)
{
    int index = findMbox(mbox_id);

    WaitingProcess* target = mailboxes[index].waitingProcsHead;

    while (target != NULL)
    {
        if (target->blockedCode == code)
        {
            int targetPid = target->pid;

            if (slot != NULL)
            {
                slot->receiverPid = targetPid;
            }

            if (mailboxes[index].slotCount != 0)
            {

                removeWaitingProc(index, targetPid);
            }

            unblock(targetPid);
            return;
        }

        target = target->pNextProcess;
    }
}


static void reorderMailSlots(int index)
{
    disableInterrupts();
    // No need to reorder if the mailbox only has 0 or 1 available slots
    if (mailboxes[index].slotCount <= 1)
    {
        return;
    }

    // Find last mail slot in linked list
    MailSlot* lastSlot = mailboxes[index].pSlotListHead;
    MailSlot* emptySlot = NULL;

    while (lastSlot->pNextSlot != NULL)
    {
        if (lastSlot->inUse == 0 && emptySlot == NULL)
        {
            emptySlot = lastSlot;
        }
        lastSlot = lastSlot->pNextSlot;
    }

    if (emptySlot == NULL)
    {
        //console_output(false, "reorderMailSlots(): Could not find the empty slot.\n");
        return;
    }

    // Missing slot is at tail
    if (emptySlot == lastSlot)
    {
        return;
    }
    // Missing slot is at head
    else if (emptySlot->pPrevSlot == NULL)
    {
        mailboxes[index].pSlotListHead = emptySlot->pNextSlot;
        mailboxes[index].pSlotListHead->pPrevSlot = NULL;
    }
    // missing slot is somewhere in the middle
    else
    {
        emptySlot->pPrevSlot->pNextSlot = emptySlot->pNextSlot;
        emptySlot->pNextSlot->pPrevSlot = emptySlot->pPrevSlot;
    }

    emptySlot->pPrevSlot = lastSlot;
    emptySlot->pNextSlot = NULL;
    lastSlot->pNextSlot = emptySlot;

    enableInterrupts();
}


static WaitingProcessPtr addFreeProc(int index)
{
    if (mailboxes[index].freeProc != NULL)
    {
        console_output(false, "addFreeProc(): Function called when mailbox already has a freeProc.\n");
        stop(1);
    }

    int target = -1;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (waitingProcsTable[i].pid == 0)
        {
            target = i;
            waitingProcsTable[target].pid = k_getpid();
            waitingProcsTable[target].index = target;
            waitingProcsTable[target].blockedCode = BLOCKED_RELEASE;
            break;
        }
    }

    if (target == -1)
    {
        console_output(false, "addFreeProc(): Waiting procedures table is full. Closing program...\n");
        stop(1);
    }

    return &waitingProcsTable[target];
}

static MailSlot* findMail(int index)
{
    MailSlot* target = mailboxes[index].pSlotListHead;

    // Search for first assigned mail
    while (target != NULL && target->receiverPid != k_getpid())
    {
        target = target->pNextSlot;
    }

    // No assigned mail, search for any unassigned mail
    if (target == NULL)
    {
        target = mailboxes[index].pSlotListHead;

        while (target != NULL && target->receiverPid != 0)
        {
            target = target->pNextSlot;
        }
    }

    return target;
}
