#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <pthread.h>
#include <string.h>

#define SHMSZ 4

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

typedef struct ElementList {
    int val;
    struct ElementList * next;
} ElementList;

//Estructura para enviar los parametros necesarios a la funcion de hilos
typedef struct ConfigM {
    int id;
    int tipo;
    int umbral;
    int espacioMemoria;
    int timeMonitSeconds;
    int intervaloDataMili;
} ConfigM;

//Metodos de ElementList
void addElement(ElementList ** cabecera, int valor);
void printList(ElementList * cabecera);
int getSize(ElementList * cabecera);
float getMedia(ElementList * cabecera);
float getVarianza(ElementList * cabecera);
void crearEspacioMemoriaSensor(int espacio, SensorList * data);
void crearEspacioMemoriaInt(int espacio, int data);


//Metodos de SensorList
void printSensors(SensorList * cabecera);
void addSensoresValidos(SensorList ** cabecera, int id, int tipo, int umbral, float media, float varianza, int espacioMemoria);
void addSensores(SensorList * cabecera, int id, int tipo, int umbral, float media, float varianza, int activo, int espacioMemoria);
int encenderAlarma(SensorList * cabecera);
void compartirInfoSensores(SensorList * cabecera);



//Funcion que lee un espacio de memoria compartido (monitorea) y luego del tiempo indicado por parametro 
void *monitoriarSensor(void * params);

SensorList * validSensors;
SensorList * allSensors;

pthread_mutex_t candado;
int alarmaEncendida=0;
int num_hilos=0;
int * alarmaCompartida;

//Reglas (restricciones): No se pueden repetir tipos cooperativos!
//                        Ningun id de los sensores pueden ser 0
//                        Ningun valor monitoriado puede ser 0

int main(int argc, char *argv[]){

    char * nombre = argv[1];
    int deltaT = atoi(argv[2]);

    //Creando el espacio de memoria para la alarma
    int shmid7;
    if ((shmid7 = shmget(67853, sizeof(int), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        return(1);
    }

    if ((alarmaCompartida = shmat(shmid7, NULL, 0)) == NULL) {
        perror("shmat");
        return(1);
    }

    *alarmaCompartida=alarmaEncendida;


    //Leer cuantos hilos va a haber desde el archivo de configuracion
    FILE *fp = fopen(nombre, "r");
    if(fp != NULL){
        char line[2000];
        while(fgets(line, sizeof line, fp) != NULL){
            num_hilos++;
        }
    }else{
        printf("\n\nNo existe el archivo de configuracion!!\n\n");
        return 1;
    }
    fclose(fp);


    crearEspacioMemoriaInt(67853, alarmaEncendida);

    //Leer el archivo de configuracion y guardar los parametros que se le van a enviar a cada hilo
    if(num_hilos>0){
        ConfigM parametros[num_hilos];
        pthread_t hilos[num_hilos];

        FILE *fp = fopen(nombre, "r");
        const char s[2] = ",";
        char *token;
        int i;
        int hilo_cont = 0;

        if(fp != NULL){
            printf("\n=====================================Configuraciones de Hilos INI=====================================\n");
            char line[2000];
            while(fgets(line, sizeof line, fp) != NULL){
                token = strtok(line, s);
                for(i=0;i<6;i++){
                    if(i==0){
                        parametros[hilo_cont].id = atoi(token);
                        token = strtok(NULL,s);
                    }else if(i==1){
                        parametros[hilo_cont].tipo = atoi(token);
                        token = strtok(NULL,s);
                    }else if(i==2){
                        parametros[hilo_cont].umbral = atoi(token);
                        token = strtok(NULL,s);
                    }else if(i==3){
                        parametros[hilo_cont].espacioMemoria = atoi(token);
                        token = strtok(NULL,s);
                    }else if(i==4){
                        //parametros[hilo_cont].timeMonitSeconds = atoi(token);
                        parametros[hilo_cont].timeMonitSeconds = deltaT;
                        token = strtok(NULL,s);
                    }else if(i==5){
                        parametros[hilo_cont].intervaloDataMili = atoi(token);
                    }    
                }
                printf("id: %d, tipo: %d, umbral: %d, espacio: %d, moniseconds; %d, intervData: %d\n", parametros[hilo_cont].id
                                                                                                     , parametros[hilo_cont].tipo
                                                                                                     , parametros[hilo_cont].umbral
                                                                                                     , parametros[hilo_cont].espacioMemoria
                                                                                                     , parametros[hilo_cont].timeMonitSeconds
                                                                                                     , parametros[hilo_cont].intervaloDataMili);
                hilo_cont++;


            }
            fclose(fp);
            printf("=====================================Configuraciones de Hilos FIN=====================================\n");
        } else {
            perror("configuracion.txt");
        } 

        if(pthread_mutex_init(&candado,NULL)!=0){
            printf("No se puedo crear el mutex para hacer sincronizacion");
            return 1;
        }

        

        while(1){
            validSensors = (SensorList *) malloc(sizeof(SensorList));
            allSensors = (SensorList *) malloc(sizeof(SensorList));

            //Asegurando que cuando se crean los nodos cabecera de las lista sea cero el id por defecto porque a veces hay datos en los atributos 
            validSensors->id=0;
            validSensors->next = NULL;
            allSensors->espacioMemoria=3000;
            allSensors->id = 0;
            allSensors->next = NULL;

            for (i=0;i<num_hilos;i++){
                pthread_create(&hilos[i],NULL, monitoriarSensor, &parametros[i]);
            }

            for (i=0;i<num_hilos;i++){
                pthread_join(hilos[i],NULL);
            }

            if(allSensors->id!=0){
                printf("\nLista de todos los sensores que fueron monitoreados:");
                printSensors(allSensors);
            }else{
                printf("\nNo se monitorio ningun sensor, puede que no haya ninguno activo.\n");
            }

            if(validSensors->id!=0){
                printf("\nLista de sensores validos:");
                printSensors(validSensors);
                alarmaEncendida = encenderAlarma(validSensors);

                *alarmaCompartida=alarmaEncendida;

                if(alarmaEncendida==1){
                    printf("\nAlarma Encendida!\n\n");
                }else if(alarmaEncendida==0){
                    printf("\nAlarma Apagada!\n\n");
                }

                
            }
            compartirInfoSensores(allSensors);
        }
        
    }else{
        printf("\nNo se encontro un archivo de configuracion valido\n");
    } 
}



void *monitoriarSensor(void * params){
    ConfigM * parametros = (ConfigM *) params;
    int id = parametros->id;
    int tipo = parametros->tipo;
    int umbral = parametros->umbral;
    int espacioMemoria = parametros->espacioMemoria;
    int timeMonitSeconds = parametros->timeMonitSeconds;
    int intervaloDataMili = parametros->intervaloDataMili;

    //pthread_mutex_lock(&candado);
    //printf("\nMonitoreando sensor id: %d, tipo: %d, umbral: %d, espacioMemoria: %d, tiempoDeMonitoreo: %d\n", id,tipo,umbral,espacioMemoria, timeMonitSeconds);
    //pthread_mutex_unlock(&candado);
    ElementList * datos = (ElementList *) malloc(sizeof(ElementList));

    int shmid, *shm;

    if ((shmid = shmget(espacioMemoria, SHMSZ,  0666)) < 0) {
        addElement(&datos,-1);
        return NULL;
    }

    if ((shm = shmat(shmid, NULL, 0)) == (int *) -1) {
        addElement(&datos,-1);
        return NULL;
    }
    
    time_t endwait;
    time_t start = time(NULL);
    time_t seconds = timeMonitSeconds; 

    endwait = start + seconds;

    while (start < endwait)
    {   
        usleep(intervaloDataMili);
        if (*shm==-1){
            pthread_mutex_lock(&candado);
            printf("\nSensor con id %d detenido!!\n",id);
            pthread_mutex_unlock(&candado);
	        addElement(&datos,-1);
	        break;
        }

	    addElement(&datos, *shm);
        start = time(NULL);
    }

    close(shmid);

    float media = 0;
    float varianza = 0;

    if(tipo<5){
        media = getMedia(datos);
        varianza = getVarianza(datos);
    }else{
        media = getMedia(datos);
    }
    
    //Hay que agregarlos a las listas pero se debe sincronizar para que no haya problemas
    pthread_mutex_lock(&candado);

    time_t t;
    struct tm *tm;
    char fechayhora[20];

    t=time(NULL);
    tm=localtime(&t);
    strftime(fechayhora, 20, "%d/%m/%Y %H:%M:%S", tm);

    char * result = (char *) malloc(strlen(fechayhora)+1);
    strcpy(result,fechayhora);

    printf ("\nFecha del ultimo registro de esta alarma: %s", result);
    
    if(datos->val!=-1){//Se agrega a las 2 listas
        addSensoresValidos(&validSensors, id, tipo, umbral, media, varianza,espacioMemoria);
        addSensores(allSensors, id, tipo, umbral, media, varianza, 1,espacioMemoria);
    }else{ //Solo se agrega a la lista de todos y se le pone valido 0
        addSensores(allSensors, id, tipo, umbral, media, varianza, 0,espacioMemoria);
    }

    printf("\nData monitoriada del sensor con id %d, tipo %d, Segundos configurados de monitoreo %d: \n",id,tipo,timeMonitSeconds);
    printList(datos);

    free(datos);
    pthread_mutex_unlock(&candado);
}

//Funcion que agrega un elemento a la lista de manera ordenada y sin repetir
void addElement(ElementList ** cabecera, int valor){
    //Se referencia la lista por medio de la cabecera
    ElementList * actual = *cabecera;

    int repetido = 0;

    if(valor==0){
        return;
    }

    if(actual->val == 0){ //Valor nunca puede ser cero
        actual->val = valor;
    }else if(valor < actual->val){
        ElementList * nuevo = (ElementList *) malloc(sizeof(ElementList));
        nuevo->val = valor;
        nuevo->next = *cabecera;
        *cabecera = nuevo;
    }else if(valor > actual->val){ 
        while (actual->next != NULL) {
            if(valor > actual->val && valor < actual->next->val){
                ElementList * nuevo = (ElementList *) malloc(sizeof(ElementList));
                nuevo->val = valor;
                nuevo->next = actual->next;
                actual->next = nuevo;
                repetido = 1;
                break;
            }else if(valor == actual->next->val){
                repetido = 1;
                break;
            }
            actual = actual->next;
        }

        if(repetido == 0){
            ElementList * nuevo = (ElementList *) malloc(sizeof(ElementList));
            nuevo->val = valor;
            nuevo->next = NULL;
            actual->next = nuevo;
        }
    }
}

float getMedia(ElementList * cabecera){
    ElementList * actual = cabecera;

    float cont = 0;
    float acumulador = 0;

    while (actual->next != NULL) {
      acumulador = acumulador + actual->val;
      cont++;
      actual = actual->next;
    }
    acumulador = acumulador + actual->val;
    cont++;

    return acumulador/cont;
}

int getSize(ElementList * cabecera){
    ElementList * actual = cabecera;
    int cont = 0;
    while (actual->next != NULL) {
      cont++;
      actual = actual->next;
    }
    cont++;

    return cont;
}

float getVarianza(ElementList * cabecera){
    ElementList * actual = cabecera;

    float acumulador = 0;
    float media = getMedia(actual);
    int numElements = getSize(actual);
    
    while (actual->next != NULL) {
      acumulador = acumulador + ((actual->val-media)*((actual->val-media)));
      actual = actual->next;
    }
    acumulador = acumulador + ((actual->val-media)*((actual->val-media)));
   
    return acumulador/numElements;
}


void printList(ElementList * cabecera){
    ElementList * actual = cabecera;
    
    while (actual->next != NULL) {
        if(actual->val!=0){
            printf("%d -> ", actual->val);
        }
        
        actual = actual->next;
    }

    if(actual->val!=0){
        printf("%d", actual->val);
    }
    
    printf("\n");
}

void printSensors(SensorList * cabecera){
    printf("\n=====================================Lista INI=====================================\n");
    SensorList * actual = cabecera;
    if(actual->id == 0 ){
        printf("Lista de sensores vacia\n");
        printf("=====================================Lista FIN=====================================\n");
        return;
    }

    while (actual->next != NULL) {
        printf("id: %d, tipo: %d, umbral: %d, media: %.2f, varianza: %.2f, activo: %d\n",   actual->id, 
                                                                                            actual->tipo, 
                                                                                            actual->umbral, 
                                                                                            actual->media, 
                                                                                            actual->varianza,
                                                                                            actual->activo);
        actual = actual->next;
    }
    printf("id: %d, tipo: %d, umbral: %d, media: %.2f, varianza: %.2f, activo: %d\n",   actual->id, 
                                                                                        actual->tipo, 
                                                                                        actual->umbral, 
                                                                                        actual->media, 
                                                                                        actual->varianza,
                                                                                        actual->activo);
    printf("=====================================Lista FIN=====================================\n");
}


//Esta funcion agrega en una lista los datos de manera ordenada por tipo sin repetir considerando la varianza y nunca se agregaran sensores inactivos (con data en la lista igual a -1)
//Al recorrer la lista que se llena con esta funcion se podra decidir si la alarma se enciende o no
void addSensoresValidos(SensorList ** cabecera, int id, int tipo, int umbral, float media, float varianza, int espacioMemoria){
    SensorList * actual = *cabecera;

    int repetido = 0;

    if(actual->id == 0){ //Caso que la lista este vacia simplemente se agrega (id cero no puede haber)
        actual->id = id;
        actual->tipo = tipo;
        actual->umbral = umbral;
        actual->media = media;
        actual->varianza = varianza;
        actual->espacioMemoria = espacioMemoria;
        actual->activo = 1;
    }else if(tipo< actual->tipo){ //si el tipo es menor se agrega
        SensorList * nuevo = (SensorList *) malloc(sizeof(SensorList));
        nuevo->id = id;
        nuevo->tipo = tipo;
        nuevo->umbral = umbral;
        nuevo->media = media;
        nuevo->varianza = varianza;
        nuevo->activo = 1;
        nuevo->espacioMemoria =espacioMemoria;
        nuevo->next = *cabecera;
        *cabecera = nuevo;
    }else if (tipo == actual->tipo && varianza < actual->varianza){ // si el tipo es igual y la varianza es menor entonces se agrega y se libera la cabecera sin perder el siguiente nodo
        
        SensorList * tmp = actual;
        SensorList * nuevo = (SensorList *) malloc(sizeof(SensorList));

        nuevo->id = id;
        nuevo->tipo = tipo;
        nuevo->umbral = umbral;
        nuevo->media = media;
        nuevo->varianza = varianza;
        nuevo->activo = 1;
        nuevo->espacioMemoria =espacioMemoria;
        nuevo->next = actual->next;
        *cabecera = nuevo;
        free(tmp);

    }else if(tipo > actual->tipo){
        while (actual->next != NULL) {
            if(tipo > actual->tipo && tipo < actual->next->tipo){
                SensorList * nuevo = (SensorList *) malloc(sizeof(SensorList));
                nuevo->id = id;
                nuevo->tipo = tipo;
                nuevo->umbral = umbral;
                nuevo->media = media;
                nuevo->varianza = varianza;
                nuevo->activo = 1;
                nuevo->espacioMemoria =espacioMemoria;
                nuevo->next = actual->next;
                actual->next = nuevo;
                repetido = 1;
                break;
            }else if(tipo == actual->next->tipo){
                repetido = 1;
                if(varianza < actual->next->varianza){ //Agrego el nodo nuevo a la lista y saco el anterior
                    SensorList * tmp = actual->next;
                    SensorList * nuevo = (SensorList *) malloc(sizeof(SensorList));
                    nuevo->id = id;
                    nuevo->tipo = tipo;
                    nuevo->umbral = umbral;
                    nuevo->media = media;
                    nuevo->varianza = varianza;
                    nuevo->espacioMemoria =espacioMemoria;
                    nuevo->activo = 1;
                    nuevo->next= actual->next->next;
                    actual->next = nuevo;
                    free(tmp);
                }
                break;
            }
            actual = actual->next;
        }

        //Si no hay repetidos y este elemento es el mayor entonces lo agrego al final de la lista
        if(repetido == 0){
            SensorList * nuevo = (SensorList *) malloc(sizeof(SensorList));
            nuevo->id = id;
            nuevo->tipo = tipo;
            nuevo->umbral = umbral;
            nuevo->media = media;
            nuevo->varianza = varianza;
            nuevo->espacioMemoria =espacioMemoria;
            nuevo->activo = 1;
            nuevo->next = NULL;
            actual->next = nuevo;
        }
    }
}


void addSensores(SensorList * cabecera, int id, int tipo, int umbral, float media, float varianza, int activo, int espacioMemoria){
    SensorList * actual = cabecera;

    if(actual->id == 0){ //Caso que la lista este vacia simplemente se agrega (id cero no puede haber)
        actual->id = id;
        actual->tipo = tipo;
        actual->umbral = umbral;
        actual->media = media;
        actual->varianza = varianza;
        actual->espacioMemoria=espacioMemoria;
        actual->activo = activo;
    }else{
        while (actual->next != NULL) {
            actual = actual->next;
        }

        SensorList * nuevo = (SensorList *) malloc(sizeof(SensorList));
        nuevo->id = id;
        nuevo->tipo = tipo;
        nuevo->umbral = umbral;
        nuevo->media = media;
        nuevo->varianza = varianza;
        nuevo->espacioMemoria =espacioMemoria;
        nuevo->activo = activo;
        nuevo->next = NULL;
        actual->next = nuevo;
    }
}

int encenderAlarma(SensorList * cabecera){
    //return 0 --> No se enciende la alarma
    //return 1 --> Se enciende la alarma

    SensorList * actual = cabecera;

    ElementList * datosHead = (ElementList *) malloc(sizeof(ElementList));
    datosHead->val=-1;
    datosHead->next=NULL;

    ElementList * datosIterator = datosHead;

    if(actual->id == 0){ 
        return 0;
    }else{
        while (actual != NULL) {
            if(actual->tipo < 5 && actual->media > actual->umbral){
                actual->cumpleRegla =0;
                return 1;
            }else if(actual->tipo>=5){
                //Tengo que ingresar los datos de los sensores cooperativos en una lista que tendra ceros y unos 
                if(datosIterator->val==-1){
                    
                    if(actual->media > actual->umbral){
                        datosIterator->val=1;
                    }else{
                        datosIterator->val=0;
                    }
                }else{
                    //Tengo que llegar al ultimo elementos de la lista
                    while (datosIterator->next != NULL) {
                        datosIterator=datosIterator->next;
                    }

                    if(actual->media > actual->umbral){
                        ElementList * nuevo = (ElementList *) malloc(sizeof(ElementList));
                        nuevo->val=1;
                        nuevo->next=NULL;
                        datosIterator->next=nuevo;
                        datosIterator=nuevo;
                    }else{
                        ElementList * nuevo = (ElementList *) malloc(sizeof(ElementList));
                        nuevo->val=0;
                        nuevo->next=NULL;
                        datosIterator->next=nuevo;
                        datosIterator=nuevo;
                    }
                }

            }
            actual=actual->next;
        }
    }

    printf("\nDatos de los sensores cooperativos:\n");
    ElementList * mostrar = datosHead;
    
    while (mostrar->next != NULL) {
        printf("%d -> ", mostrar->val);
       
        mostrar = mostrar->next;
    }

        printf("%d", mostrar->val);
    
    printf("\n");

    float media = 0;

    media=getMedia(datosHead);

    printf("\nScorp: %.2f\n", media);

    if(media>0.7){
        return 1;
    }

    return 0;
}

void crearEspacioMemoriaInt(int espacio, int data){
    int shmid2;

    int * shmValor;

    if ((shmid2 = shmget(espacio, sizeof(int), IPC_CREAT | 0666)) < 0) {
        printf("Error al hacer shmget el shmid2 es: %d\n",shmid2);
        perror("shmget");
        return;
    }

    if ((shmValor = shmat(shmid2, NULL, 0)) == NULL) {
        printf("Error al hacer shmat\n");
        perror("shmat");
        return;
    }

    *shmValor=data;
}

void crearEspacioMemoriaSensor(int espacio, SensorList * data){
    int shmid2;

    SensorList * shmSensor;

    if ((shmid2 = shmget(espacio, sizeof(SensorList), IPC_CREAT | 0666)) < 0) {
        printf("Error al hacer shmget el shmid2 es: %d\n",shmid2);
        perror("shmget");
        return;
    }

    if ((shmSensor = shmat(shmid2, NULL, 0)) == NULL) {
        printf("Error al hacer shmat\n");
        perror("shmat");
        return;
    }

    shmSensor->id=data->id;
    shmSensor->activo=data->activo;
    shmSensor->media=data->media;
    shmSensor->next=data->next;
    shmSensor->umbral=data->umbral;
    shmSensor->varianza=data->varianza;
    shmSensor->tipo=data->tipo;
    if(data->media > data->umbral){
        shmSensor->cumpleRegla=0;
    }else{
        shmSensor->cumpleRegla=1;
    }
    shmSensor->espacioMemoria =data->espacioMemoria;
    

}


void compartirInfoSensores(SensorList * cabecera){
    SensorList * actual = cabecera;
    if(actual->id == 0 ){
        return;
    }

    //Para poner la lista de los sensores en espacio de memoria compartido
    int comm2=66688;
    int cont= 1;
    

    while (actual->next != NULL) {
        crearEspacioMemoriaSensor(comm2,actual);
        comm2 = comm2 + 10;
        actual = actual->next;
        cont++;
    }
    
    crearEspacioMemoriaSensor(comm2,actual);

    


    crearEspacioMemoriaInt(77725,cont);



}