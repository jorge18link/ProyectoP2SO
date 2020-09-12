Existen espacios de memoria que estan por defecto para el funcionamiento de monitor.c:

1: El espacio de memoria con key 77725 almacena la cantidad de sensores que Existen
2: El espacio de memoria con key 66688 almacena el resultado de monitoreo del uno de los sensores (Los espacios key aumenta de 10 en 10 tomando la cantidad de sensores)
3: El espacio de memoria con key 67853 el estado de la alarma 0 para apagada y 1 para encendida


El archivo de configuracion tiene que tener la siguiente estructura:

1,3,500,2601,1000000
2,3,500,2611,1000000
3,7,500,2621,1000000
4,10,500,2631,1000000
5,5,500,2641,1000000

Donde las comas separan columnas y cada columna representa lo siguiente:
1: El id del sensor
2: El tipo del sensor
3: El umbral
4: El espacio de memoria donde se genera la data (Importante: por cada registro tienen que estar de 10 en 10 y no se tiene que dejar espacios en blanco ni lineas vacias)
5: Este dato representa el intervalo de tiempo en millis en que se genera data en el espacio de memoria de la columna anterior


Pasos para ejecutar monitor.c:

1: ejecutar --> sh -x iniciarSensores.sh [nombre de archivo de configuracion]
2: ejecutar --> gcc proyectoSATP2.c -lpthread -o proyectoSO_SATP2
3: ejecutar --> ./proyectoSO_SATP2 [nombre de archivo de configuracion] [tiempo DeltaT]
4: esperar un ciclo DeltaT para que se creen los espacios de memoria compartidos
5: ejecutar --> gcc monitor.c -o monitor
6: ejecutar --> ./monitor

El programa monitor tiene un menu con las siguientes opciones:

MENU DE OPCIONES 
    1.-Monitorear data de los sensores
    2.-Mostrar Resultado de Sensores
    3.-Mostrar Estado de la alarma
    4. Salir.

La opcion 1 contiene lo que se pedia en el literal "a" de requerimientos del proyecto (Los datos que llegan desde cada uno de los sensores)
La opcion 2 contiene lo que se pedia en el literal "b" y "c" de requerimientos del proyecto (Las regla que se cumplen y Ver información de diagnostico de sensores)
La opcion 3 contiene lo que se pedia en el literal "b" de requerimientos del proyecto (El estado de la alarma)
La opcion 4 es para salir del monitor.
