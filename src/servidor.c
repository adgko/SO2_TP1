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
//#include <openssl/md5.h>
#include <unistd.h>
#include <errno.h>

#define TAM 256
#define USUARIO_TAM 20
#define USUARIO_BLOQUEADO_TAM 2
#define LIMITE_INTENTOS 3
#define COMANDO_TAM 32

#define LOGIN_ID 1
#define LOGIN_ACCEPT 2
#define LISTA_NOMBRES 3
#define LISTA_NOMBRES_RESPUESTA 4
#define PASSWORD_CHANGE 5
#define PASSWORD_CHANGE_RESPONSE 6
#define LISTA_ARCHIVOS 7
#define ARCHIVOS_RESPUESTA 8
#define DESCARGA_IMAGEN 9
#define DESCARGA_IMAGEN_RESPONSE 10

#define direccion_server "src/servidor.c"

struct msgbuf {
   long mtype;
   char mtext[TAM];
};

int32_t send_to_queue(long id_message, char message[TAM]);
char* recive_from_queue(long id_mensaje, int32_t flag);


int32_t main( int32_t argc, char *argv[] ) {
	int32_t sockfd, newsockfd, puerto, clilen, pid;
	char buffer[TAM];
	struct sockaddr_in serv_addr, cli_addr;
	int32_t n;
	char* mensaje_str;
	int auth_flag = 0;	

	if ( argc < 2 ) {
        	fprintf( stderr, "Uso: %s <puerto>\n", argv[0] );
		exit( 1 );
	}

	sockfd = socket( AF_INET, SOCK_STREAM, 0);
	if ( sockfd < 0 ) { 
		perror( " apertura de socket ");
		exit( 1 );
	}

	memset( (char *) &serv_addr, 0, sizeof(serv_addr) );
	puerto = atoi( argv[1] );
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons( puerto );

	if ( bind(sockfd, ( struct sockaddr *) &serv_addr, sizeof( serv_addr ) ) < 0 ) {
		perror( "ligadura" );
		exit( 1 );
	}

        printf( "Proceso: %d - socket disponible: %d\n", getpid(), ntohs(serv_addr.sin_port) );

	listen( sockfd, 5 );
	clilen = sizeof( cli_addr );

	pid = fork();
  		if ( pid == 0 ) {
    		if( execv("bin/auth",  argv) == -1 ) {
   			  perror("error en auth ");
   			  exit(1);
   			}    			
   			exit(0);
  		}
  		pid = fork();
  		if ( pid == 0 ) {
    		if( execv("bin/file",  argv) == -1 ) {
   			  perror("error en file ");
   			  exit(1);
   			}    			
   			exit(0);
  		}

	while( 1 ) {
		newsockfd = accept( sockfd, (struct sockaddr *) &cli_addr, &clilen ); //conexión con el cliente 
		if ( newsockfd < 0 ) {
			perror( "accept" );
			exit( 1 );
		}
		char user[USUARIO_TAM];
		
  		

		pid = fork(); 
		if ( pid < 0 ) {
			perror( "fork" );
			exit( 1 );
		}

		if ( pid == 0 ) {  // Proceso hijo
			close( sockfd );

			while ( 1 ) {
				memset( buffer, 0, TAM );
					n = read( newsockfd, buffer, TAM-1 );		//recepción de usuario y contraseña por parte del cliente en caso de auth_flag=0
					if ( n < 0 ) {								//recepción de comandos en caso de auth_flag=1
						perror( "lectura de socket" );
						exit(1);
					}

					if( strcmp(buffer, "exit\n") == 0 ) {
						printf("Saliendo...\n");
						break;
					}

				if(auth_flag == 0){
					
					if(send_to_queue((long) LOGIN_ID, buffer) == -1) {
						printf("error enviando mensaje\n");
						exit(1);
					}

					mensaje_str = recive_from_queue(LOGIN_ACCEPT, 0);


				
					if( mensaje_str[0] == '0') {					//si auth dice "0", es que el usuario está bloqueado
						char* nombre = strtok(buffer, "-");
						printf("USUARIO BLOQUEADO: %s\n", nombre);
						n = write( newsockfd, "0", 18 );			//y le mando a cleinte el código "0" que indica bloqueado
						if ( n < 0 ) {
							perror( "escritura en socket" );
							exit( 1 );
						}
						auth_flag=0;
					}
					else if(mensaje_str[0] == '1') {
						sprintf(user, "%s", strtok(buffer, "-"));
						printf("USUARIO ACEPTADO: %s\n", user);
						n = write( newsockfd, "1", 18 );			//y le mando a cleinte el código "0" que indica bloqueado
						if ( n < 0 ) {
							perror( "escritura en socket" );
							exit( 1 );
						}
						auth_flag=1;
					}
					else{
						char* nombre = strtok(buffer, "-");
						printf("ERROR DE AUTENTICACIÓN: %s\n", nombre);
						n = write( newsockfd, "2", 18 );			//y le mando a cleinte el código "0" que indica bloqueado
						if ( n < 0 ) {
							perror( "escritura en socket" );
							exit( 1 );
						}
						auth_flag=0;
					}
				}
				else{
					buffer[strlen(buffer)-1] = '\0';

					char* mensaje;
					char opcion[COMANDO_TAM];
					char argumento[COMANDO_TAM];
					char comando[COMANDO_TAM];

					int32_t i = 0;

					mensaje = strtok(buffer, " ");
					sprintf(opcion, " ");
					sprintf(argumento, " ");

					while(mensaje != NULL) {
						if(i == 0) {
							sprintf(comando, "%s", mensaje);
							i++;
						}
						else if(i == 1) {
							sprintf(opcion, "%s", mensaje);
							i++;
						}
						else {
							sprintf(argumento, "%s", mensaje)	;
							break;
						}
						mensaje = strtok(NULL, " ");
					}

					printf( "%s - %s %s %s\n", user, comando, opcion, argumento);

					if( strcmp("exit", comando) == 0 ){
						printf("Usuario salió\n");
						break;
					}
					else if( strcmp("user", comando) == 0 ){
						//lista de usarios y última conexión
						if( strcmp("ls", opcion) == 0 ){								
							if(send_to_queue((long) LISTA_NOMBRES, "nombres") == -1) {
								printf( "error enviando mensaje\n");
								exit(1);
							}
							mensaje_str = recive_from_queue(LISTA_NOMBRES_RESPUESTA, 0); // respuesta de auth_service

							char respuesta_envio[strlen(mensaje_str)];
							if(respuesta_envio == NULL) {
								printf("error alocando memoria\n");
								exit(1);
							}
							sprintf(respuesta_envio, "%s", mensaje_str);

							n = write( newsockfd, respuesta_envio, 18 );			//le envío la respuesta del auth al cliente
								if ( n < 0 ) {
									perror( "escritura en socket" );
									exit( 1 );
								}
						}
						//cambiar clave
						else if( strcmp("passwd", opcion) == 0 && strcmp(" ", argumento) != 0)	
							if( strlen(argumento) == 0 || strlen(argumento) > USUARIO_TAM) {	
								n = write( newsockfd, "clave invalida\n", 18 );			
								if ( n < 0 ) {
									perror( "escritura en socket" );
									exit( 1 );
								}
							}
							char* guion = "-";
							char temp[strlen(user) + strlen(argumento) + strlen(guion)];
							sprintf(temp,"%s%s%s", user, guion, argumento);
							if(send_to_queue((long) PASSWORD_CHANGE, temp) == -1) {
								printf("error enviando mensaje\n");
								exit(1);
							}
							recive_from_queue((long) PASSWORD_CHANGE_RESPONSE, 0);

							n = write( newsockfd, "Se ha modificado la clave\n", 18 );			
								if ( n < 0 ) {
									perror( "escritura en socket" );
									exit( 1 );
								}

						else {
							n = write( newsockfd, " Comandos válidos: \n"
												"user [opcion] <argumento>\n\n"
												"	- ls : listado de usuarios\n"
												"	- passwd <nueva contraseña> : cambio de contraseña", 18 );			
								if ( n < 0 ) {
									perror( "escritura en socket" );
									exit( 1 );
								}
						}
					}
					else if( strcmp("file", comando) == 0 ){
						if( strcmp("ls", opcion) == 0 )
							if(send_to_queue((long) LISTA_ARCHIVOS, "i") == -1) {
									printf("error enviando mensaje\n");
									exit(1);
								}
							mensaje_str = recive_from_queue(ARCHIVOS_RESPUESTA, 0);
							char respuesta_envio[strlen(mensaje_str)];

							sprintf(respuesta_envio, "%s", mensaje_str);

							n = write( newsockfd, respuesta_envio, 18 );			
								if ( n < 0 ) {
									perror( "escritura en socket" );
									exit( 1 );
								}

						else if( strcmp("down", opcion) == 0 && strcmp(" ", argumento) != 0){
							if(send_to_queue((long) DESCARGA_IMAGEN, argumento) == -1) {
										printf("error enviando mensaje\n");
										exit(1);
							}
							mensaje_str = recive_from_queue(DESCARGA_IMAGEN_RESPONSE, 0);

							char respuesta_envio[strlen(mensaje_str)];
							sprintf(respuesta_envio, "%s", mensaje_str);

							n = write( newsockfd, respuesta_envio, 18 );			
								if ( n < 0 ) {
									perror( "escritura en socket" );
									exit( 1 );
								}
							}

						else {
							n = write( newsockfd, " Comandos válidos: \n"
												"file [opcion] <argumento>\n\n"
												"	- ls : listado de archivos\n"
												"	- down <nombre_archivo> : descarga el archivo", 18 );			
								if ( n < 0 ) {
									perror( "escritura en socket" );
									exit( 1 );
								}
						}
					}
					else{
						n = write( newsockfd, " Comandos inválido: \n"
											  " Comandos válidos: \n"
												"user \n"
												"file \n"
												"exit", 18 );			
							if ( n < 0 ) {
								perror( "escritura en socket" );
								exit( 1 );
							}
					}


				}
			}
				
	}
	exit(0); 
} 
}

int32_t get_queue() {

  key_t key;
  key = ftok(direccion_server, 66);								// proj_id = 66

  if (key == -1) {
    perror("error obteniendo token");
    exit(1);
  }

  return msgget(key, 0666 | IPC_CREAT);							//para conseguir el file descriptor, uso la key y la otra parte
  																// 0666 me da permiso y IPC_CREAT crea el espacio de memoria
}

int32_t send_to_queue(long id_message, char message[TAM]) {		//id_message es el tipo de mensaje y message es el mensaje en si

  if(strlen(message) > TAM) {
    perror("mensaje muy grande\n");
    exit(1);
  }
  struct msgbuf mensaje_str;
  mensaje_str.mtype = id_message;
  sprintf(mensaje_str.mtext, "%s", message);
  return msgsnd(get_queue(), &mensaje_str, sizeof mensaje_str.mtext, 0 );
}

char* recive_from_queue(long id_message, int32_t flag) {		//id_message es el tipo de mensaje y flag lo uso para recibir
  errno = 0;
  struct msgbuf mensaje_str = {id_message, {0}};

  if(msgrcv(get_queue(), &mensaje_str, sizeof mensaje_str.mtext, id_message, flag) == -1) {
      if(errno != ENOMSG) {
        perror("error recibiendo mensaje de cola");
        exit(1);
      }
  }
  char* mensaje = malloc(strlen(mensaje_str.mtext));
  sprintf(mensaje, mensaje_str.mtext);
  return mensaje;
}












