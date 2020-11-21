#include "../include/recursos.h"

/*
	Funciones empleadas por Server
*/
void configurar_socket();
void ejecutar_bin();
void escuchando();
void conectar_cliente();
void set_flags();
void rec_user();
void enviar_a_cola_local(long , char [MENSAJE_TAM]);
void enviar_a_cliente(char*);
void verificar_respuesta();
void inter();
void validar_comando();
void exit_command();
void user_command( char*, char*);
void user_ls();
void user_passwd(char*);
void file_command(char*, char*);
void file_ls();
void file_down(char*);
void envio_mensaje_aux();
void unknown_command();

/*
	declaración de variables
	sacadas del ejemplo de socket
	agrego auth_flag, con lo que sabré si el cliente está logueado
	exit_flag para saber si el cliente quiere salir
	on_flag para saber si el programa tiene que seguir corriendo
*/
int32_t sockfd, sock_cli, puerto;
int32_t pid_auth, pid_file;
ssize_t n;									// hubo que declarar n como ssize_t para que no pierda información al usarse en send() y recv()
struct sockaddr_in serv_addr;
struct sockaddr_in client_addr;
struct hostent *server;
struct hostent *server_file;
int terminar;
char buffer[TAM], direccion[20];
int auth_flag, exit_flag, on_flag;					//bandera autenticación, salida y funcionamiento
int32_t rta;								//respuesta del server al cliente
uint32_t client_len;						//tamaño de la dirección del cliente
char* mensaje_resp;
char user[USUARIO_TAM];