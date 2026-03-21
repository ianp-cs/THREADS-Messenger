#pragma once


typedef struct waiting_process *WaitingProcessPtr;
typedef struct mail_slot *SlotPtr;
typedef struct mailbox MailBox;

typedef enum {MB_ZEROSLOT=0, MB_SINGLESLOT, MB_MULTISLOT, MB_MAXTYPES} MAILBOX_TYPE;
typedef enum {MBSTATUS_EMPTY=0, MBSTATUS_INUSE, MBSTATUS_RELEASED, MBSTATUS_MAX} MAILBOX_STATUS;

/* Block status values for use with block() */
#define BLOCKED_RECEIVE 11
#define BLOCKED_SEND    12
#define BLOCKED_RELEASE 13

typedef struct mail_slot 
{
   SlotPtr			pNextSlot;
   SlotPtr			pPrevSlot;
   int				mbox_id;				// ID of the mailbox this slot is assigned to
   unsigned char	message[MAX_MESSAGE];	
   int				messageSize;
   int				inUse;					// Contains a message to be received
   int				senderPid;				// The pid of the sending process
   int				receiverPid;			// The pid of the receiving process

} MailSlot;

typedef struct waiting_process
{
   WaitingProcessPtr    pNextProcess;
   WaitingProcessPtr    pPrevProcess;
   int					blockedCode;			
   int                  pid;
   int					index;
   unsigned char		message[MAX_MESSAGE];	// Used in zero slot mailboxes to send a direct message
   int					messageSize;			// Used in zero slot mailboxes to send a direct message
} WaitingProcess;

struct mailbox 
{
   SlotPtr				pSlotListHead;					// Points to the first mail slot in the mailbox. Is rotated as messages are received from mailboxes
   int					mbox_id;
   MAILBOX_TYPE			type;
   MAILBOX_STATUS		status;
   int					slotSize;
   int					slotCount;
   int					fullSlots;						// Number of slots that have been filled with messages
   WaitingProcessPtr	waitingProcsHead;				// Pointer to the first process waiting on this mailbox
   int					waitingProcsCount;				// Number of waiting processes, excluding the freeProc
   WaitingProcessPtr	freeProc;						// Pointer to the process in charge of freeing this mailbox
};



