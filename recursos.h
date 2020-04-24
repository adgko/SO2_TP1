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
#include <dirent.h>
#include <time.h>
#include <openssl/md5.h>
#include <unistd.h>

#define TAM 256
#define USUARIO_TAM 20
#define USUARIO_BLOQUEADO_TAM 2
#define LIMITE_INTENTOS 3
