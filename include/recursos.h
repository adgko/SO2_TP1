#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
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


#define TAM 256
#define USUARIO_TAM 20
#define USUARIO_BLOQUEADO_TAM 2
#define LIMITE_INTENTOS 3
#define MENSAJE_TAM 254
#define COMANDO_TAM 32
#define CLAVE_TAM 12
#define CANTIDAD_USUARIOS 4
#define RENGLON 20
#define LAST_CONECTION_SIZE 32
#define INTENTOS_TAM 5
#define USUARIO_CAMPOS 4
#define TIME_SLEEP 2

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
#define AUTH_PATH "bin/auth"
#define FILE_PATH "bin/file"

#define PUERTO_FILE 1051

#define CANT_ARCHIVOS 3
#define ARCHIVO_NAME_SIZE 128
#define ARCHIVO_FORMAT_SIZE 8
#define BYTES_TO_MB 1048576

#define KNRM  "\x1B[0m"		//normal
#define KRED  "\x1B[31m"	//rojo
#define KGRN  "\x1B[32m"	//verde
#define KYEL  "\x1B[33m"	//amarillo
#define KBLU  "\x1B[34m"	//azul
#define KMAG  "\x1B[35m"	//magenta
#define KCYN  "\x1B[36m"	//cyan
#define KWHT  "\x1B[37m"	//blanco

#define QUEUE_NAME "src/servidor.c"
#define PROJ_ID 112
#define direccion_server "src/servidor.c"


struct msgbuf {
   long mtype;
   char mtext[TAM];
};

int32_t get_queue();
int32_t send_to_queue(long, char [MENSAJE_TAM] );
char* recive_from_queue(long , int32_t );
