#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/un.h>
#include <limits.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <openssl/md5.h>

/*
	Tamaños de arreglos usados
*/
#define TAM 256
#define USUARIO_TAM 20
#define USUARIO_BLOQUEADO_TAM 2
#define LIMITE_INTENTOS 3
#define MENSAJE_TAM 1024
#define COMANDO_TAM 32
#define CLAVE_TAM 64
#define CANTIDAD_USUARIOS 4
#define RENGLON 20
#define LAST_CONECTION_SIZE 64
#define INTENTOS_TAM 5
#define USUARIO_CAMPOS 4
#define TIME_SLEEP 2
#define TIME_SLEEP_MICRO 800

/*
	Etiquetas empleadas en la cola de mensaje
*/
#define LOGIN_REQUEST 1
#define LOGIN_RESPONSE 2
#define NAMES_REQUEST 3
#define NAMES_RESPONSE 4
#define PASSWORD_CHANGE 5
#define PASSWORD_CHANGE_RESPONSE 6
#define FILES_REQUEST 7
#define FILES_RESPONSE 8
#define DOWNLOAD_REQUEST 9
#define DOWNLOAD_RESPONSE 10

/*
	paths para los ejecutables
*/
#define AUTH_PATH "auth"
#define FILE_PATH "file"


#define USUARIO_ERROR 0
#define USUARIO_ACEPTADO 1
#define USUARIO_BLOQUEADO 2

/*
	Elementos empleados en el pasaje de archivos
*/
#define PUERTO_FILE 1051
#define CANT_ARCHIVOS 3
#define ARCHIVO_NAME_SIZE 128
#define ARCHIVO_FORMAT_SIZE 8
#define BYTES_TO_MB 1048576

/*
	Variables empleadas para imprimir en colores
*/
#define KNRM  "\x1B[0m"		//normal
#define KRED  "\x1B[31m"	//rojo
#define KGRN  "\x1B[32m"	//verde
#define KYEL  "\x1B[33m"	//amarillo
#define KBLU  "\x1B[34m"	//azul
#define KMAG  "\x1B[35m"	//magenta
#define KCYN  "\x1B[36m"	//cyan
#define KWHT  "\x1B[37m"	//blanco

/*
	Elementos empleados para crear y usar la cola de mensajes
*/
#define QUEUE_NAME "../archivos/queue"
#define UNIQUE_KEY 65
#define PROJ_ID 0666
#define direccion_server "src/servidor.c"

/*
	Estructura del buffer de la cola de mensajes
*/
struct msgbuf {
   long mtype;
   char mtext[TAM];
};

/*
	Declaración de Funciones
*/
int32_t get_queue();
int32_t send_to_queue(long, char [MENSAJE_TAM] );
char* recive_from_queue(long , int32_t );
char* get_MD5(char* );