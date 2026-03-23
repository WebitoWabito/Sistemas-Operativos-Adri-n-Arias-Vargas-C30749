/**
  *   C++ program to send messages using passing message encapsulation class
  *
  *   UCR-ECCI
  *
  *   CI0122 Sistemas Operativos 2026-i
  *
  *   enviarConClases.cc
  *
 **/

#define LABEL_SIZE 64
#include <stdio.h>
#include <string.h>

#include "Buzon.h"

struct msgbuf {
   long mtype;
   int times;
   char label[LABEL_SIZE];
};

const char * html_labels[] = {
   "a",
   "b",
   "c",
   "d",
   "e",
   "li",
   ""
};

int main( int argc, char ** argv ) {


   int i, st;
   Buzon m;
   struct msgbuf msg;

   i = 0;

   while ( strlen(html_labels[ i ] ) ) {
      msg.mtype = 2026;
      msg.times = i;
      strncpy(msg.label, html_labels[i], LABEL_SIZE);
      msg.label[LABEL_SIZE-1] = '\0';
      st = m.Enviar( ((char*)&msg) + sizeof(long), sizeof(msg) - sizeof(long), 2026 );
      printf("Label: %s, status %d \n", html_labels[ i ], st );
      i++;
   }

}