/* printtest.c
 *	Simple program to test whether printing from a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

int
main()
{
	system_PrintString("hello world\n");
  system_PrintInt(3);
/*  
    system_PrintString("Executed\n ");
    system_Testing();
    system_PrintString("Executed\n ");
//system_Sleep(30);
    system_PrintInt(system_GetNumInstr());
//    return 0;
int i;*/

//system_PrintString("\n\n\n\n\n");
//system_PrintInt(system_Fork());
//system_PrintString("\n\n\n\n\n");
int g;
    g=system_Fork();
	system_PrintString("Executed\n ");
	system_PrintInt(g);
/*if (g==0)
{
	system_PrintString("Executed\n ");
	//while(1);
}*/
/*
else
{
	system_PrintInt(g);
	system_PrintString("fello world\n");
	while(1);
}
while(1);*/
//	for(i=0;i<10000;i++) {
// system_PrintInt(i);
//system_PrintString("\n");
//if(i%2==0) system_Sleep(10000000);
//}
/*    system_PrintString("instructions.\n");

    system_PrintInt(system_GetNumInstr());*/
}
