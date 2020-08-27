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

*/
void get_MD5(char* archivo,char* buffer){

	printf("Probando MD5 6\n");
	unsigned char c[MD5_DIGEST_LENGTH];
	FILE *inFile = fopen (archivo, "rb");
	MD5_CTX mdContext;
	size_t bytes;
	unsigned char data[1024];


	if (inFile == NULL) {
		printf ("%s no se pudo abrir el archivo%s\n", KRED,KNRM);
		exit(1);
	}
		
	printf("Probando MD5 7\n");
	MD5_Init (&mdContext);
	while ((bytes = fread (data, 1, 1024, inFile)) != 0)
		MD5_Update (&mdContext, data, bytes);
	MD5_Final (c,&mdContext);

	memset(buffer,0,MD5_DIGEST_LENGTH*2);

	printf("Probando MD5 8\n");
	for(int32_t i = 0; i < MD5_DIGEST_LENGTH; i++) {
		char aux[strlen(buffer)];
		sprintf(aux,"%s",buffer);
		sprintf(buffer,"%s%02x",aux,(unsigned int)c[i]);
		printf("%s\n", buffer);	
	}

	printf("Probando MD5 9\n");
	buffer[strlen(buffer)]='\0';

	//char* resp_aux = malloc(MD5_DIGEST_LENGTH);
	//sprintf(resp_aux,"%s",respuesta);
	//free(respuesta);
	fclose (inFile);

	printf("%s\n",buffer );
	printf("Probando MD5 10\n");
	//return resp_aux;
}