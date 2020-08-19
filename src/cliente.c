#include "../include/cliente.h"

/*
	declaración de variables
	sacadas del ejemplo de socket
	agrego auth_flag, con lo que sabré si el cliente está logueado
*/
int32_t sockfd, sockfil, puerto1, puerto2;
ssize_t n;									// hubo que declarar n como ssize_t para que no pierda información al usarse en send() y recv()
struct sockaddr_in serv_addr;
struct sockaddr_in serv_addr_file;
struct hostent *server;
struct hostent *server_file;
int terminar = 0;
char buffer[TAM];
int auth_flag = 0;
int32_t rta = 0;

int main(int argc, char *argv[])
{

	if ( argc < 3 ) {
		fprintf( stderr, "Uso %s host puerto\n", argv[0]);
		exit( 0 );
	}
	
	/*
	Puerto1 es por donde pasa los datos
	Puerto2 es por donde recibe las imagenes para el pendrive
	*/
	puerto1 = atoi( argv[2] );
	puerto2 = puerto1 + 1;
	server = gethostbyname( argv[1] );
	server_file = gethostbyname( argv[1] );

	conect_to_server();			// configura el socket y se conecta con el servidor

	signal_handler();			//configuro el handler para que apunte a salida
	
	while(1) {

			/*
				Si auth_flag es 0, no hay usuarios autenticados, por lo que pedirá credenciales hasta que encuentre alguno o se bloquee
			*/
			if(auth_flag == 0){
				rta = login();

				if(rta == 0)
					printf("%scredenciales erroneas, vuelva a intentar%s\n",KYEL,KNRM);
				else if(rta == 1) {
					printf("%sConectado%s\n",KGRN,KNRM);
					auth_flag = 1;
				}
				else if(rta == 2) {
					printf("\n%sUsuario bloqueado%s\n\n",KRED,KNRM);
					salida(0);
				}

			}

			/*
				Si auth_flag es 1, un usario se logueó y puede enviar comandos
			*/
			else{
				comandos();
				if(buffer[0] != '\0'){		//por si el mensaje es nulo

					if(strcmp(buffer, "exit\n") == 0){	//si se escribe "exit", cierra sesión y cierra el programa
						salida(1);
					}

					recibir_respuesta(sockfd);	

					if(strcmp(buffer, "descarga_no") == 0){
						printf("%sNo se encuentra el archivo%s\n",KYEL,KNRM);
						printf("%sAsegurese de que este bien escrito%s\n",KYEL,KNRM);
					}
					else if(strcmp(buffer, "descarga_si") == 0) {
						descargar();
					}
					else {
						printf("%s\n", buffer);
						fflush(stdout);
					}		
				}
			}
	}

	return 0;
}

/*
	Abre conexión con el socket server
*/
void conect_to_server()
{
	sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sockfd < 0 ) {
		perror( "ERROR apertura de socket" );
		exit( 1 );
	}
	memset( (char *) &serv_addr, '0', sizeof(serv_addr) );
	serv_addr.sin_family = AF_INET;
	bcopy( (char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, (size_t)server->h_length );		// hay que castear server->h_length para que fucnione en bcopy
	serv_addr.sin_port = htons( (uint16_t)puerto1 );
	if ( connect( sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr ) ) < 0 ) {
		perror( "conexion" );
		exit( 1 );
	}
}

/*
	Cuando tocamos Ctrl+C o escribimos exit en consola,
	viene acá
*/
void signal_handler()
{
	struct sigaction sign;
	sign.sa_handler = salida;
	sigaction(SIGINT, &sign,  NULL);
}
void salida(int32_t exit_flag) {
	if(exit_flag == 1){								// como y aestaba logueado, avisa al server que se va
		printf("%sVuelvas prontoss%s \n",KBLU,KNRM);
		fflush(stdout);
		enviar_a_socket(sockfd, "exit\n");
		}		
	exit(0);
}

/*
	Sirve para enviar las credenciales usuario y contraseña al server
*/
void enviar_a_socket(int32_t socket, char* mensaje) {
	n = send( socket, mensaje, strlen(mensaje), 0 );
	if ( n < 0 ) {
	  perror( "error enviando a socket\n");
	  exit( 1 );
	}
}

/*
	Loguea al usuario, y obtiene una respuesta sobre si pudo, no o está bloqueado
*/
int32_t login(){
	char usuario[TAM];
	char password[TAM];

	printf("Ingrese su usuario:\n");
    fgets(usuario,TAM,stdin);
    printf("Ingrese su contraseña:\n");
    fgets(password,TAM,stdin);

    char* cad_aux = malloc(strlen(usuario) + strlen(password));
    if(cad_aux == NULL) {
		perror("error de alocación\n");
		exit(1);
	}
	sprintf(cad_aux, "%s%s", usuario, password);			// almacena todos los valores en cad_aux
	/*
		envío el usuario y contraseña al server y libero la memoria 
		para que no se solape si tengo que ingresar los datos de nuevo
	*/
	enviar_a_socket(sockfd,cad_aux);	
	free(cad_aux);

	/*
		si el server escribe un 1 en el buffer, se logueo correctamente. 
		Si escribe un 0, falló y pide nuevamente las credenciales
		Si escribe cualquier otra cosa, el usuario está bloqueado.
	*/
	if( buffer[0] == '1' )						
		return 1;
	else if( buffer[0] == '0' )
		return 0;
	else{
		printf("%s\n",buffer );
		return 2;
	}
}

void comandos(){
	while(1){
		memset(buffer, '\0', TAM);			// con esto limpio el buffer del comando anterior
		printf("$");
		fgets(buffer,TAM,stdin); 			// pido comando
		if(buffer[0] != '\n'){				//si en el buffer no hay una nueva linea, envía
			enviar_comando();
		}else{
			break;
		}
	}
}

/*
	Es lo mismo que enviar_a_socket() pero siempre envía lo que hay en el buffer
*/
void enviar_comando(){							
	n = send( sockfd, buffer, strlen(buffer), 0 );
	if ( n < 0 ) {
	  perror( "error de envío\n");
	  exit( 1 );
	}
}
void recibir_respuesta(int32_t socket) {
	memset( buffer,0,TAM);
	n = recv( socket,buffer,TAM,0);
	if ( n < 0 ) {
	  perror( "error de recepción\n" );
	  exit(1);
	}
}

void descargar(){
	conect_to_files();

	

}

/*
	Es lo mismo que connect_to_server, pero para file, 
	así le pasa el archivo que quiere descargar por otro socket distinto
	al que usa para enviar comandos
*/
void conect_to_files(){
	sockfil = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sockfd < 0 ) {
		perror( "ERROR apertura de socket" );
		exit( 1 );
	}
	memset( (char *) &serv_addr_file, '0', sizeof(serv_addr) );
	serv_addr_file.sin_family = AF_INET;
	bcopy( (char *)server_file->h_addr, (char *)&serv_addr_file.sin_addr.s_addr, (size_t)server_file->h_length );		// hay que castear server->h_length para que fucnione en bcopy
	serv_addr_file.sin_port = htons( (uint16_t)puerto2 );
	if ( connect( sockfd, (struct sockaddr *)&serv_addr_file, sizeof(serv_addr_file ) ) < 0 ) {
		perror( "conexion" );
		exit( 1 );
	}
}