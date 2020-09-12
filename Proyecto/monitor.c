#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> 

typedef struct SensorList {
    int id;
    int tipo;
    int umbral;
    float media;
    float varianza;
    int activo; //1 activo, 0 inactivo
    int cumpleRegla; //1 si 0 no
    int espacioMemoria;
    struct SensorList * next;
} SensorList;

int comm;
int shmid;
int shmid2;
int shmid3;
int shmid4;


SensorList * shm;
SensorList * shm2;
int * cantidadSensores;
int * alarma;
SensorList ** sensores;
int cant;

void mostrarMenuListarSensores();
void mostrarSensoresMonitoriados();
void mostrarDataSensor(int espacioMemoria, int timeSeconds);
void mostrarAlarma();
void mostrarMenu();

int main() {
    
    if ((shmid3 = shmget(77725, sizeof(int),  0666)) < 0) {
        printf("\nNo se pudo obtener el espacio de memoria!\n");
        return(1);
    }

    if ((cantidadSensores = shmat(shmid3, NULL, 0)) == NULL) {
        printf("\nNo se pudo obtener el espacio de memoria!\n");
        return(1);
    }
    cant = *cantidadSensores;

    mostrarMenu();

    return 0;
}

void mostrarMenu(){
    system("clear");
    int opcion=0;
    printf("\n\t\t MENU DE OPCIONES ");
    printf("\n\t 1.-Monitorear data de los sensores");
    printf("\n\t 2.-Mostrar Resultado de Sensores");
    printf("\n\t 3.-Mostrar Estado de la alarma");
    printf("\n\t 4. Salir." );
    printf("\nIntroduzca opcion (1-4): ");

    do{
        scanf( "%d", &opcion );
        if(opcion<1 || opcion >4){
            printf("\nIntroduzca una opcion valida(1-3): ");
        }
    }while (opcion<1 || opcion >4);
    

    switch ( opcion ){
        case 1:
            system("clear");

            mostrarMenuListarSensores();
            
            printf("\nIntroduzca 1 para regresar al menu o cualquier otra cosa para salir: ");
            scanf( "%d", &opcion );
            if(opcion==1){
                system("clear");
                mostrarMenu();
                return;
            }

            break;
        case 2:
            system("clear");

            mostrarSensoresMonitoriados();

            printf("\nIntroduzca 1 para regresar al menu o cualquier otra cosa para salir: ");
            scanf( "%d", &opcion );
            if(opcion==1){
                system("clear");
                mostrarMenu();
                return;
            }
            break;
        case 3:
            system("clear");

            mostrarAlarma();

            printf("\nIntroduzca 1 para regresar al menu o cualquier otra cosa para salir: ");
            scanf( "%d", &opcion );
            if(opcion==1){
                system("clear");
                mostrarMenu();
                return;
            }
            break;
        case 4:
            printf("\nHasta Luego\n\n");
            system("clear");
            return;
            break;
    }
}


void mostrarAlarma(){
    int shmid, *shm;

    system("clear");
    if ((shmid = shmget(67853, sizeof(int),  0666)) < 0) {
        printf("\nEspacio de memoria invalido\n");
        return;
    }

    if ((shm = shmat(shmid, NULL, 0)) == (int *) -1) {
        printf("\nEspacio de memoria invalido\n");
        return;
    }

    if(*shm==0){
        printf("\nLa alarma esta apagada\n");
    }else{
        printf("\nLa alarma esta Encencida\n");
    }
}

void mostrarMenuListarSensores(){
    int shmid2;
    SensorList * shmSensor;
    int espacioInicial=66688;
    int i = 0;
    int opcion;
    int opcion2;

    printf("\n\t\tLISTA DE SENSORES: ");

    for (i = 0; i < cant; i++){
        if ((shmid2 = shmget(espacioInicial, sizeof(SensorList), IPC_CREAT | 0666)) < 0) {
            printf("Error al hacer shmget el shmid2 es: %d\n",shmid2);
            perror("shmget");
            return;
        }

        if ((shmSensor = shmat(shmid2, NULL, 0)) == NULL) {
            printf("Error al hacer shmat\n");
            perror("shmat");
            return;
        }
        printf("\n\tSensor con id: %d, tipo: %d, umbral: %d (%d)",  shmSensor->id, 
                                                                    shmSensor->tipo, 
                                                                    shmSensor->umbral,
                                                                    shmSensor->espacioMemoria);

        espacioInicial=espacioInicial+10;
    }

    printf("\n\nIntroduzca opcion (Lo que esta en parentisis o -1 para el menu principal): ");
    scanf( "%d", &opcion );

    if(opcion==-1){
        mostrarMenu();
        return;
    }

    printf("\n\n Durante cuanto tiempo quiere ver la data del sensor (segundos): ");
    scanf( "%d", &opcion2);
    mostrarDataSensor(opcion,opcion2);

}

void mostrarDataSensor(int espacioMemoria, int timeSeconds){
    int shmid, *shm;

    system("clear");
    if ((shmid = shmget(espacioMemoria, sizeof(int),  0666)) < 0) {
        printf("\nEspacio de memoria invalido\n");
        return;
    }

    if ((shm = shmat(shmid, NULL, 0)) == (int *) -1) {
        printf("\nEspacio de memoria invalido\n");
        return;
    }
    
    time_t endwait;
    time_t start = time(NULL);
    time_t seconds = timeSeconds; 

    endwait = start + seconds;

    int first = 1;

    while (start < endwait)
    {   

        
        if (*shm==-1){
            printf("\nEste sensor esta detenido\n");
	        break;
        }

        printf("%d\n", *shm);

        start = time(NULL);
        usleep(1000000);
    }
}

void mostrarSensoresMonitoriados(){
    int shmid2;
    SensorList * shmSensor;
    int espacioInicial=66688;
    int i = 0;

    for (i = 0; i < cant; i++){
        if ((shmid2 = shmget(espacioInicial, sizeof(SensorList), IPC_CREAT | 0666)) < 0) {
            printf("Error al hacer shmget el shmid2 es: %d\n",shmid2);
            perror("shmget");
            return;
        }

        if ((shmSensor = shmat(shmid2, NULL, 0)) == NULL) {
            printf("Error al hacer shmat\n");
            perror("shmat");
            return;
        }
        printf("id: %d, tipo: %d, umbral: %d, media: %.2f, varianza: %.2f, activo: %d, cumpleRegla: %d, espacioData: %d\n",   shmSensor->id, 
                                                                                            shmSensor->tipo, 
                                                                                            shmSensor->umbral, 
                                                                                            shmSensor->media, 
                                                                                            shmSensor->varianza,
                                                                                            shmSensor->activo,
                                                                                            shmSensor->cumpleRegla,
                                                                                            shmSensor->espacioMemoria);

        espacioInicial=espacioInicial+10;
    }
}