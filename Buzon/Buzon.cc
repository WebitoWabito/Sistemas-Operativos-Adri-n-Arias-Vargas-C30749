/**
  *   C++ class to encapsulate Unix message passing intrinsic structures and system calls
  *
  *   UCR-ECCI
  *
  *   CI0122 Sistemas Operativos 2026-i
  *
  *   Class implementation
  *
 **/

#include <stdexcept>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <cstring>

#include "Buzon.h"


/**
  *  Class constructor
  *
 **/
Buzon::Buzon() {
   int st = msgget(KEY, IPC_CREAT | 0600);
   if ( -1 == st ) {
      throw std::runtime_error( "Buzon::Buzon( int )" );
   }
   id = st;
}


/**
  * Class destructor
  *
 **/
Buzon::~Buzon() {
}


/**
  *  Send method
  *
  *  @param     const char * mensaje: arreglo de caracteres a enviar
  *
 **/
int Buzon::Enviar( const char * mensaje, long tipo ) {
   struct {
      long mtype;
      char mtext[256];
   } msg;
   msg.mtype = tipo;
   strncpy(msg.mtext, mensaje, sizeof(msg.mtext));
   int st = msgsnd(id, &msg, sizeof(msg.mtext), 0);
   if ( -1 == st ) {
      throw std::runtime_error( "Buzon::Enviar( const char * )" );
   }
   return st;
}


/**
  *  Send method
  *
  *  @param     const void * mensaje: estructura con el mensaje a enviar
  *  @param	int cantidad: cantidad de bytes a enviar
  *
  *
 **/
int Buzon::Enviar(const void *mensaje, int cantidad, long tipo) {
    struct {
        long mtype;
        char mtext[4096];
    } msg;
    msg.mtype = tipo;
    if (cantidad > (int)sizeof(msg.mtext)) cantidad = sizeof(msg.mtext);
    memcpy(msg.mtext, mensaje, cantidad);
    int st = msgsnd(id, &msg, cantidad, 0);
    if (-1 == st) {
        throw std::runtime_error("Buzon::Enviar( const void *, int, long )");
    }
    return st;
}


/**
  *  Receive method
  *
  *  @param     const void * mensaje: estructura con el mensaje a enviar
  *  @param	int cantidad: cantidad de bytes a enviar
  *
  *
 **/
int Buzon::Recibir(void *mensaje, int cantidad, long tipo) {
    struct {
        long mtype;
        char mtext[4096];
    } msg;
    if (cantidad > (int)sizeof(msg.mtext)) cantidad = sizeof(msg.mtext);
    int st = msgrcv(id, &msg, cantidad, tipo, 0);
    if (-1 == st) {
        return st;
    }
    memcpy(mensaje, msg.mtext, cantidad);
    return st;
}