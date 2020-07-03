#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <signal.h>
#define TAM 256

/*
	declaración de variables
	sacadas del ejemplo de socket
	agrego auth_flag, con lo que sabré si el cliente está logueado
*/
int sockfd, puerto1, puerto2, n;
struct sockaddr_in serv_addr;
struct sockaddr_in serv_addr_file;
struct hostent *server;
struct hostent *server_file;
int terminar = 0;
char buffer[TAM];
int auth_flag = 0;
struct sigaction sign;


void conect_to_server();
void signal_handler();
void salida();

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

			if(auth_flag == 0){
				printf( "Ingrese el mensaje a transmitir: " );
				memset( buffer, '\0', TAM );
				fgets( buffer, TAM-1, stdin );

				n = send( sockfd, buffer, strlen(buffer), 0);
				if ( n < 0 ) {
					perror( "escritura de socket" );
					exit( 1 );
				}

				// Verificando si se escribió: fin
				buffer[strlen(buffer)-1] = '\0';
				if( !strcmp( "fin", buffer ) ) {
				terminar = 1;
				}

				memset( buffer, '\0', TAM );
				n = recv( sockfd, buffer, TAM, 0 );
				if ( n < 0 ) {
					perror( "lectura de socket" );
					exit( 1 );
				}
				printf( "Respuesta: %s\n", buffer );
				if( terminar ) {
					printf( "Finalizando ejecución\n" );
					exit(0);
				}
			}
			else{
				printf( "Ingrese el mensaje a transmitir: " );
				memset( buffer, '\0', TAM );
				fgets( buffer, TAM-1, stdin );

				n = send( sockfd, buffer, strlen(buffer), 0);
				if ( n < 0 ) {
					perror( "escritura de socket" );
					exit( 1 );
				}

				// Verificando si se escribió: fin
				buffer[strlen(buffer)-1] = '\0';
				if( !strcmp( "fin", buffer ) ) {
				terminar = 1;
				}

				memset( buffer, '\0', TAM );
				n = recv( sockfd, buffer, TAM, 0);
				if ( n < 0 ) {
					perror( "lectura de socket" );
					exit( 1 );
				}
				printf( "Respuesta: %s\n", buffer );
				if( terminar ) {
					printf( "Finalizando ejecución\n" );
					exit(0);
				}
			}
	}

	return 0;
}

void conect_to_server()
{
	sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sockfd < 0 ) {
		perror( "ERROR apertura de socket" );
		exit( 1 );
	}
	memset( (char *) &serv_addr, '0', sizeof(serv_addr) );
	serv_addr.sin_family = AF_INET;
	bcopy( (char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length );
	serv_addr.sin_port = htons( (uint32_t)puerto1 );
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
	sign.sa_handler = salida;
	sigaction(SIGINT, &sign,  NULL);
}
void salida() {
	printf("Vuelvas prontoss \n");
	fflush(stdout);
	//enviar_a_socket(socket, "exit\n");
	exit(0);
}

void enviar_a_socket(int32_t socket, char* mensaje) {
	n = send( socket, mensaje, strlen(mensaje), 0 );
	if ( n < 0 ) {
	  perror( "error enviando a socket\n");
	  exit( 1 );
	}
}