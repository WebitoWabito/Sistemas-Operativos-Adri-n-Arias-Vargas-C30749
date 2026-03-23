/**
  *   C++ class to encapsulate Unix message passing intrinsic structures and system calls
  *
  *   UCR-ECCI
  *
  *   CI0122 Sistemas Operativos 2026-i
  *
  *   Class interface
  *
 **/

#define KEY 0xA54321	// Valor de la llave del recurso

class Buzon {
   public:
      Buzon();
      ~Buzon();
      int Enviar( const char *mensaje, long = 1 );
      int Enviar( const void *mensaje, int, long = 1 );
      int Recibir( void *mensaje, int, long = 1 );	// len: space in mensaje

   private:
      int id;		// Identificador del buzon
};
