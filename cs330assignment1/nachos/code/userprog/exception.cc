// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "console.h"
#include "synch.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }
extern void StartProcess(char *file);
static void ConvertIntToHex (unsigned v, Console *console)
{
   unsigned x;
   if (v == 0) return;
   ConvertIntToHex (v/16, console);
   x = v % 16;
   if (x < 10) {
      writeDone->P() ;
      console->PutChar('0'+x);
   }
   else {
      writeDone->P() ;
      console->PutChar('a'+x-10);
   }
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int memval, vaddr, printval, tempval, exp;
    unsigned printvalus;        // Used for printing in hex
    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);;

    if ((which == SyscallException) && (type == syscall_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == syscall_PrintInt)) {
       printval = machine->ReadRegister(4);
       if (printval == 0) {
	  writeDone->P() ;
          console->PutChar('0');
       }
       else {
          if (printval < 0) {
	     writeDone->P() ;
             console->PutChar('-');
             printval = -printval;
          }
          tempval = printval;
          exp=1;
          while (tempval != 0) {
             tempval = tempval/10;
             exp = exp*10;
          }
          exp = exp/10;
          while (exp > 0) {
	     writeDone->P() ;
             console->PutChar('0'+(printval/exp));
             printval = printval % exp;
             exp = exp/10;
          }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintChar)) {
	writeDone->P() ;
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintString)) {
	vaddr = machine->ReadRegister(4);
       machine->ReadMem(vaddr, 1, &memval);
       while ((*(char*)&memval) != '\0') {
	  writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;
          machine->ReadMem(vaddr, 1, &memval);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintIntHex)) {
       printvalus = (unsigned)machine->ReadRegister(4);
       writeDone->P() ;
       console->PutChar('0');
       writeDone->P() ;
       console->PutChar('x');
       if (printvalus == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          ConvertIntToHex (printvalus, console);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
 else if ((which == SyscallException) && (type == syscall_GetReg)) {

	int query_reg = machine->ReadRegister(4);
	if (query_reg >= NumTotalRegs || query_reg < 0)
		ASSERT(FALSE);	
//	printf("here%d\n",query_reg);
//	machine->WriteRegister(query_reg, 234); 
	int ans_reg=machine->ReadRegister(query_reg);
//	printf("sdgfsd%d\n",ans_reg);
	machine->WriteRegister(2, ans_reg);	
       // Advance program counters.
	machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
 else if ((which == SyscallException) && (type == syscall_GetPID)) {

        machine->WriteRegister(2, currentThread->getPID());
       // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
 else if ((which == SyscallException) && (type == syscall_GetPPID)) {

        machine->WriteRegister(2, currentThread->getPPID());
       // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
 else if ((which == SyscallException) && (type == syscall_Time)) {

        machine->WriteRegister(2, stats->totalTicks);
       // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
 else if ((which == SyscallException) && (type == syscall_Exec)) {
	vaddr = machine->ReadRegister(4);
      char tempprog[100];int i=0;
	 machine->ReadMem(vaddr, 1, &memval);
       while ((*(char*)&memval) != '\0') {
     	tempprog[i]=(*(char*)&memval) ;
          vaddr++;i++;
          machine->ReadMem(vaddr, 1, &memval);
       }
	tempprog[i]='\0';
//currentThread->space->~AddrSpace();
	StartProcess(tempprog);
       // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

 else if ((which == SyscallException) && (type == syscall_NumInstr)) {

        machine->WriteRegister(2, currentThread->numinstr + (stats->systemTicks-currentThread->sstamp)/SystemTick + (stats->userTicks-currentThread->ustamp)/UserTick);
       // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

 else if ((which == SyscallException) && (type == syscall_Fork)) {

        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
     machine->WriteRegister(2, currentThread->CreateChild());
       // Advance program counters.
      //printf("%d\n",machine->registers[2]);
    }
 
 else if ((which == SyscallException) && (type == syscall_Join)) {
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

	int waitaddr = machine->ReadRegister(4);
//        NachOSThread * ptr = (NachOSThread *)progList->FindInList(waitaddr);
    if (progarray[waitaddr]->getPPID()!=currentThread->getPID())
    {
       machine->WriteRegister(2, -1);
    }
    else{
    	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	if(progarray[waitaddr]==NULL) {machine->WriteRegister(2, returnaddresses[waitaddr]);(void) interrupt->SetLevel(oldLevel);}
	else {

		waitingforchild[currentThread->getPID()]=waitaddr;
   		
   		currentThread->PutThreadToSleep();
	   	(void) interrupt->SetLevel(oldLevel);
		machine->WriteRegister(2, returnaddresses[waitaddr]);
	}
      
    }
       // Advance program counters.
    }
     else if ((which == SyscallException) && (type == syscall_Sleep)) {
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

  int sleeptime = (unsigned)machine->ReadRegister(4);
  if (sleeptime == 0) currentThread->YieldCPU ();
  else
  {
    scheduler->ScheduledSleep(sleeptime);
  }
       // Advance program counters.
    }
 else if ((which == SyscallException) && (type == syscall_Yield)) {
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

	currentThread->YieldCPU ();
	   /* NachOSThread *nextThread;
	    IntStatus oldLevel = interrupt->SetLevel(IntOff);
	
	    nextThread = scheduler->FindNextToRun();
	    if (nextThread != NULL) {
	        scheduler->ReadyToRun(currentThread);
	        scheduler->Run(nextThread);
	    } */

       // Advance program counters.
    }

 else if ((which == SyscallException) && (type == syscall_Exit)) {

	
           /* NachOSThread *nextThread;
            IntStatus oldLevel = interrupt->SetLevel(IntOff);
        
            nextThread = scheduler->FindNextToRun();
            if (nextThread != NULL) {
                scheduler->ReadyToRun(currentThread);
                scheduler->Run(nextThread);
            } */

       // Advance program counters.
	int i;
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
	returnaddresses[currentThread->getPID()]=machine->ReadRegister(4);
	if (progarray[currentThread->getPPID()]!=NULL&&waitingforchild[currentThread->getPPID()]==currentThread->getPID())
	{
		waitingforchild[currentThread->getPPID()]=0;
		scheduler->ReadyToRun(progarray[currentThread->getPPID()]);
	}
	for(i=0;i<=pidcount;i++)
	{
		if(progarray[i]!=NULL&&progarray[i]->getPPID()==currentThread->getPID()) progarray[i]->setPPID(0);
	}
        currentThread->FinishThread ();
    }


 else if ((which == SyscallException) && (type == syscall_GetPA)) {
	int query=machine->ReadRegister(4);
	TranslationEntry *entry;
	int i;
		for (entry = NULL, i = 0; i < TLBSize; i++)
        		    if (machine->tlb[i].valid && (machine->tlb[i].virtualPage ==query)&&(machine->tlb[i].physicalPage<NumPhysPages)) {
				entry = &machine->tlb[i];                        // FOUND!
	                	break;	
	        	    }
		        if (entry == NULL) {                            // not found
			    machine->WriteRegister(2, -1);
                                                // the page may be in memory,
                                                // but not in the TLB
	        	}

			else{
			       	 machine->WriteRegister(2, machine->tlb[i].physicalPage);
			}
 	

       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }



 else if ((which == SyscallException) && (type == syscall_Testing)) {
        writeDone->P() ;
        console->PutChar('P');
        writeDone->P() ;
        console->PutChar('a');
        writeDone->P() ;
        console->PutChar('s');
        writeDone->P() ;
        console->PutChar('s');
        writeDone->P() ;
        console->PutChar('e');
        writeDone->P() ;
        console->PutChar('d');
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

 

 else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
