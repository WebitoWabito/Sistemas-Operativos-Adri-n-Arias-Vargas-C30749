#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#include "Buzon.h"


#define TIPO_MUTEX 1L
#define TIPO_LISTO 2L
#define TIPO_PERMISO(i) (100L + (i))

#define MAX_CALLES 16

struct MsgMutex {
    int n_total;
    int colas[MAX_CALLES];
};

struct MsgListo {
    int calle;
};


void proceso_carro(Buzon &buzon, int carro_id, int calle_id, int M, int N) {
    // se toma el mutex
    MsgMutex estado;
    buzon.Recibir(&estado, sizeof(estado), TIPO_MUTEX);

    estado.n_total++;//se registra la llegada del carro
    estado.colas[calle_id]++;

    // congestion, si n_total>N, se busca la calle con Q_max y se reubica
    int calle_espera = calle_id;
    if (estado.n_total > N) {
        int max_cola = -1;
        for (int i = 0; i < M; i++) {
            if (estado.colas[i] > max_cola) {
                max_cola = estado.colas[i];
                calle_espera = i;
            }
        }
        if (calle_espera != calle_id) {
            estado.colas[calle_id]--;
            estado.colas[calle_espera]++;
            std::cout << "Carro " << carro_id << ": Congestion detectada (" << estado.n_total << " carros en total). " << "Se mueve de calle " << calle_id << " a calle " << calle_espera << std::endl;
        }
    }

    buzon.Enviar(&estado, sizeof(estado), TIPO_MUTEX);
    MsgListo listo;
    listo.calle = calle_espera;
    buzon.Enviar(&listo, sizeof(listo), TIPO_LISTO);

    int perm = 0;
    buzon.Recibir(&perm, sizeof(perm), TIPO_PERMISO(calle_espera));

    //ingreso a la rotonda
    std::cout << "Carro " << carro_id << ": Ingreso a la rotonda por calle "<< calle_espera << "." << std::endl;
}


// SE reciben los avisos de los carros y se les otorga permiso de ingreso según la calle con Q_max
void proceso_rotonda(Buzon &buzon, int M, int total_carros) {
    int colas[MAX_CALLES];
    memset(colas, 0, sizeof(colas));

    //todos los carros indican la calle en la que quedaron formados
    for (int i = 0; i < total_carros; i++) {
        MsgListo listo;
        buzon.Recibir(&listo, sizeof(listo), TIPO_LISTO);
        colas[listo.calle]++;
    }

    std::cout << "\nRotonda: Todos los carros estan formados." << std::endl;
    for (int i = 0; i < M; i++) {
        std::cout << "Calle " << i << ": " << colas[i] << " carros" << std::endl;
    }
    std::cout << std::endl;

    // se da el permiso de paso siempre a la calle con mas carros esperando
    int pasados = 0;
    while (pasados < total_carros) {
        int calle_elegida = -1;
        int max_cola = 0;
        for (int i = 0; i < M; i++) {
            if (colas[i] > max_cola) {
                max_cola = colas[i];
                calle_elegida = i;
            }
        }
        if (calle_elegida >= 0) {
            colas[calle_elegida]--;
            int perm = 1;
            buzon.Enviar(&perm, sizeof(perm), TIPO_PERMISO(calle_elegida));
            pasados++;
        }
    }
}


int main(int argc, char *argv[]) {

    if (argc < 4) {
        std::cerr << "Uso: " << argv[0] << " <M calles> <N umbral> <num_carros>" << std::endl;
        return 1;
    }

    int M          = atoi(argv[1]);
    int N          = atoi(argv[2]);
    int num_carros = atoi(argv[3]);

    if (M <= 0 || M > MAX_CALLES) {
        std::cerr << "Error: M debe estar entre 1 y " << MAX_CALLES << std::endl;
        return 1;
    }

    std::cout << "Calles: " << M << " Umbral N: " << N << " Carros: " << num_carros << std::endl << std::endl;

    // se limpia cualquier cola de mensajes huerfana antes de iniciar la simulacion
    int id_viejo = msgget(KEY, 0600);
    if (id_viejo != -1) {
        msgctl(id_viejo, IPC_RMID, nullptr);
    }
    Buzon buzon;

    // estado inicial, con 0 carros y las colas vacias
    MsgMutex estadoInicial;
    estadoInicial.n_total = 0;
    memset(estadoInicial.colas, 0, sizeof(estadoInicial.colas));
    buzon.Enviar(&estadoInicial, sizeof(estadoInicial), TIPO_MUTEX);

    // se crea el gestor
    pid_t pid_gestor = fork();
    if (pid_gestor == 0) {
        proceso_rotonda(buzon, M, num_carros);
        exit(0);
    }

    // procesos para cada carro
    for (int i = 0; i < num_carros; i++) {
        int calle = i % M;
        pid_t pid = fork();
        if (pid == 0) {
            usleep(i * 2000);
            proceso_carro(buzon, i + 1, calle, M, N);
            exit(0);
        } else if (pid < 0) {
            std::cerr << "Error creando proceso para carro " << i + 1 << std::endl;
        }
    }
    for (int i = 0; i < num_carros + 1; i++) {
        wait(nullptr);
    }

    std::cout << "Fin de la simulación" << std::endl;
    return 0;
}