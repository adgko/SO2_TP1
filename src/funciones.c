#include "../include/recursos.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                   COLA DE MENSAJE
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	crea la cola si no existe y devuelve el id
*/
int32_t get_queue() {

  key_t qkey;
  qkey = ftok(QUEUE_NAME, UNIQUE_KEY);		//devuelve una key desde el path y los 8 bits menos significativos de PROJ_ID

  if (qkey == -1) {
    perror("error obteniendo token");
    exit(1);
  }

  return msgget(qkey, PROJ_ID|IPC_CREAT);				//devuelve el id de la cola asociada a qkey. IPC_CREATE sirve para que falle si la cola ya existe
}

/*
	Se emplea para enviar los mensajes a la cola, 
	con la que se comunica server, file y auth
*/
int32_t send_to_queue(long id_mensaje, char mensaje[MENSAJE_TAM]) {

	int32_t msqid=get_queue();
  if(strlen(mensaje) > MENSAJE_TAM) {
    perror("error, mensaje muy grande\n");
    exit(1);
  }
  struct msgbuf mensaje_str;
  mensaje_str.mtype = id_mensaje;
  sprintf(mensaje_str.mtext, "%s", mensaje);

  return msgsnd(msqid, &mensaje_str, sizeof mensaje_str.mtext, 0 );
}

char* recive_from_queue(long id_mensaje, int32_t flag) {
  int32_t msqid=get_queue();
  errno = 0;			//seteo en 0 para que no se pise cada vez que lo llamo
  struct msgbuf mensaje_str = {id_mensaje, {0}};
  
  if(msgrcv(msqid, &mensaje_str, sizeof mensaje_str.mtext, id_mensaje, flag) == -1) {
      if(errno != ENOMSG) {
        perror("error recibiendo mensaje de cola");
        exit(1);
      }
  }
  char* mensaje = malloc(strlen(mensaje_str.mtext));
  sprintf(mensaje,"%s", mensaje_str.mtext);
  return mensaje;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//								MD5
/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
	Función que calcula el MD5
	recorre el archivo del path seleccionado realizando el cálculo
	Si el size pasado por parámetro es 0, significa que está levantando el archivo por primera vez
		en ese casolo lee completamente
	Si el size pasado por parámetro es un número positivo, es el límite que tiene que leer
*/
char *get_MD5(char path[TAM], size_t size)
{
  FILE *file = fopen(path, "rb");

  char buffer[TAM];
  size_t bytes, bytes_read;

  unsigned char c[MD5_DIGEST_LENGTH];
  MD5_CTX md_context;

  MD5_Init(&md_context);

  if(size == 0)
    {
      while((bytes = fread(buffer, sizeof(char), sizeof(buffer), file)) != 0)
        MD5_Update(&md_context, buffer, bytes);
    }
  else
    {
      bytes = 0;
      while(bytes < size)
        {
          bytes_read = fread(buffer, sizeof(char), sizeof(buffer), file);
          MD5_Update(&md_context, buffer, bytes_read);
          bytes += bytes_read;
        }
    }

  MD5_Final(c, &md_context);
  fclose(file);

  char *md5 = malloc(MD5_DIGEST_LENGTH * 2 + 1);

  for(int32_t i = 0; i < MD5_DIGEST_LENGTH; i++)
    sprintf(&md5[i*2], "%02x", (uint) c[i]);

  return md5;
}

