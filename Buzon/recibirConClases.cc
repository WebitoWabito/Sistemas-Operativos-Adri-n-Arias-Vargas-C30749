/**
  *   C++ program to receive messages from a mailbox, using normal C++ classes 
  *
  *   UCR-ECCI
  *
  *   CI0122 Sistemas Operativos 2026-i
  *
  *   recibirConClases.c  (direct system call to message passing)
  *
 **/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Buzon.h"

#define LABEL_SIZE 64


int main( int argc, char ** argv ) {


struct msgbuf {
       long mtype;
       int times;
       char label[ LABEL_SIZE ];
};

   struct msgbuf A;
   int st;
   Buzon m;
   st = m.Recibir( ((char*)&A) + sizeof(long), sizeof(A) - sizeof(long), 2026 );
   while ( st > 0 ) {
      printf("Label: %s, times %d \n", A.label, A.times );
      st = m.Recibir( ((char*)&A) + sizeof(long), sizeof(A) - sizeof(long), 2026 );
   }

}