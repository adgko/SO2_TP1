#include "recursos.h"

/*
	Funciones empleadas en el Cliente
*/
void conf_puertos();
void conect_to_server();
void signal_handler();
void salida();
void enviar_a_socket(int32_t, char*);
int32_t login();
int32_t validar_login();
void validar_rta();
void comandos();
void enviar_comando();
void leer_server();
void recibir_respuesta(int32_t);
void descargar();
void conect_to_files();
void confirmar_files();


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
int32_t auth_flag = 0;
int32_t rta = 0;