#include "../include/servidor.h"


/*
	declaración de variables
	sacadas del ejemplo de socket
	agrego auth_flag, con lo que sabré si el cliente está logueado
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

int32_t main( int32_t argc, char *argv[] ){
		

	if ( argc < 2 ) {
		fprintf(stderr,"Uso: %s <direccion ip><puerto>\n", argv[0]);		
		exit( 1 );
	}

	// definicion de puerto y direccion
	sprintf(direccion, "%s", argv[1]);
	puerto = atoi( argv[2] );

	configurar_socket();
	

    printf( "Proceso: %d - socket disponible: %d\n", getpid(), ntohs(serv_addr.sin_port) );

    escuchando();		//escucha por si se conecta un cliente
	
    //ejecutar_bin();		//ejecuta los binarios de auth y file
    
    pid_auth = fork();
  		if ( pid_auth == 0 ) {
    		if( execv(AUTH_PATH, argv) == -1 ) {
   			  perror("error en auth ");
   			  exit(1);
   			} 			
   			exit(0);
  		}

  	pid_file = fork();
  		if ( pid_file == 0 ) {
    		if( execv(FILE_PATH, argv) == -1 ) {
   			  perror("error en file ");
   			  exit(1);
   			}   			
   			exit(0);
		}
	
	conectar_cliente();
	exit_flag = 0;
	auth_flag = 0;
	on_flag = 1;

	while(on_flag) {

		if(auth_flag == 0){
			printf("%sAUTENTICANDO%s\n",KGRN,KNRM);

			rec_user();				//recibo credenciales de usuario

			if( strcmp(buffer, "exit\n") == 0 ) {	//si pongo exit, sale
				printf("%sSaliendo...%s\n",KBLU,KNRM);
				break;
			}
					
			send_to_queue((long) LOGIN_REQUEST, &buffer[0]); // reenvio a auth_service
			mensaje_resp = recive_from_queue(LOGIN_RESPONSE, 0); //respuesta de auth_sevice

			verificar_respuesta(); //acá se si se logueo, no o está bloqueado
			
		}
		else{
			while(exit_flag==0){
				rec_user();
				middle();			// recibe comandos del user
			}

		}

	}
				
		
	exit(0); 
}


/*
	Abro socket para que se conecte un cliente
 */
void configurar_socket() {
	sockfd = socket( AF_INET, SOCK_STREAM, 0);
	memset( (char *) &serv_addr, 0, sizeof(serv_addr) );

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(direccion);
	serv_addr.sin_port = htons( (uint16_t) puerto );
	if ( bind(sockfd, ( struct sockaddr *) &serv_addr, sizeof( serv_addr ) ) < 0 ) {
		printf("Error de conexión\n");
		exit(1);
	}
}

/*
	Abre conexión para que se conecte un clietne
*/
void escuchando(){
	listen(sockfd, 5);
	client_len = sizeof(client_addr);
}

/*
	inicializa los procesos de autenticación y archivos
*/
/*
void ejecutar_bin(){
	pid_auth = fork();
  		if ( pid_auth == 0 ) {
    		if( execv(AUTH_PATH, (char* const*)NULL) == -1 ) {
   			  perror("error en auth ");
   			  exit(1);
   			} 			
   			exit(0);
  		}
  	pid_file = fork();
  		if ( pid_file == 0 ) {
    		if( execv(FILE_PATH, (char* const*)NULL) == -1 ) {
   			  perror("error en file ");
   			  exit(1);
   			}   			
   			exit(0);
		}
}
*/
/*
	Conecta con el cliente que usó su dirección
*/
void conectar_cliente(){
	sock_cli = accept( sockfd, (struct sockaddr *) &client_addr, &client_len ); 
	if ( sock_cli < 0 ) {
		perror( "accept" );
		exit( 1 );
	}
}

/*
	Recibe el usuario y contraseña desde cliente
*/
void rec_user(){
	memset( buffer, '\0', TAM );
	n = recv( sock_cli, buffer, TAM, 0 );	
	if ( n < 0 ) {								
		perror( "lectura de socket" );
		exit(1);
		}
}


/*
	Envía datos hacia el cliente. Usado para respuestas de login y comandos
*/
void enviar_a_cliente(char* mensaje) {
	n = send( sock_cli, mensaje, strlen(mensaje), 0 );
	if ( n < 0 ) {
		printf("%serror enviando a cliente%s\n",KRED,KNRM );
	  	exit( 1 );
	}
}

/*
	Verifica la respuesta. Ve si hay un 0 (credenciales fallidas), 
	1 (usuario logueado) o 2 (usuario bloqueado)
*/
void verificar_respuesta(){
	if( mensaje_resp[0] == '0') {					//si auth dice "0", es que el usuario o la contraseña está mal escrito
		char* nombre = strtok(buffer, "\n");
		printf("%sINTENTO FALLIDO: %s%s\n", KYEL,nombre,KNRM);
		printf("%sRevise que este bien escrito%s\n",KYEL,KNRM);
		enviar_a_cliente("0");
		auth_flag=0;
	}
	else if(mensaje_resp[0] == '1') {				//si se loguea, guarda el user para comandos
		sprintf(user, "%s", strtok(buffer, "\n"));
		printf("%sUSUARIO ACEPTADO: %s%s\n", KGRN,user,KNRM);
		enviar_a_cliente("1");
		auth_flag=1;
	}
	else{											//si auth sale bloqueado, notifica al server
		char* nombre = strtok(buffer, "\n");
		printf("%sUSUARIO BLOQUEADO: %s%s\n", KRED,nombre,KNRM);
		enviar_a_cliente("2");
		auth_flag=0;
		exit_flag=1;
		on_flag=0;
	}
}

/*
	Obtiene el comando a enviar y funciona como intermediario
*/
void middle(){
	buffer[strlen(buffer)-1]='\0'; //coloca un valor final al final del comando

	/*
	variables que usa para guardar comandos, opciones y argumentos
	*/
	char* mensaje_comando;
	char comando[COMANDO_TAM];
	char opcion[COMANDO_TAM];
	char argumento[COMANDO_TAM];

	//separa el comando en tokens para valuar
	mensaje_comando = strtok(buffer, " ");

	for(int32_t i=0; mensaje_comando != NULL; i++){
		if(i == 0){
			sprintf(comando, "%s", mensaje_comando);
		}
		else if(i == 1){
			sprintf(opcion,"%s", mensaje_comando);
		}
		else{
			sprintf(argumento,"%s",mensaje_comando);
		}

		mensaje_comando = strtok(NULL," ");
	}

	fflush(stdout);

	printf("%s - %s %s %s\n", user, comando, opcion, argumento );

	validar_comando(&comando,&opcion,&argumento);
}	

/*
	Revisa si el comando es exit, user o file.
	Exit: invoca exit_command y sale del sistema
	User: invoca user_command para listado o cambio de pass
	File: invoca file_command para listado o descarga
	Sino, avisa que el comando no existe
*/
void validar_comando(char *a, char *b, char *c){

	if( strcmp("exit", a) == 0 ){
		exit_command();
	}
	else if( strcmp("user", a) == 0 ){
		user_command(b, c);
	}
	else if( strcmp("file", a) == 0 ){
		file_command(b, c);
	}
	else{
		unknown_command();
	}
}

/*
	Lo emplea para avisar que el usuario se desconecta
	y cambia la bandera salida para que salga del bucle en el main
*/
void exit_command() {
	printf("%sEl usuario %s se ha desconectado%s\n", KBLU,user,KNRM );
	exit_flag = 1;
	on_flag = 0;
}

/*
	Verifica si el user pide el listado (ls) o 
	cambiar password (passwd)
	De lo contrario, avisa que está mal
*/
void user_command( char *opcion, char *argumento) {
	if( strcmp("ls", opcion) == 0 ){
		user_ls();
	}
	else if( strcmp("passwd", opcion) == 0 && strcmp(" ", argumento) != 0){
		user_passwd(argumento);
	}
	else {
		enviar_a_cliente(	" Opción Incorrecta\n"
							" Escriba: user [opcion] <argumento>\n"
							"	- ls : listado de usuarios\n"
							"	- passwd <nueva contraseña> : cambio de contraseña");
	}
}

/*
	Envía a la cola local la petición del listado a auth_service y 
	recibe la respuesta.
	Si es correcta, la transmite al cliente.
*/
void user_ls() {
	send_to_queue((long) NAMES_REQUEST, "LISTA DE NOMBRES");
	mensaje_resp = recive_from_queue(NAMES_RESPONSE, 0); 

	char respuesta_envio[strlen(mensaje_resp)];
	if(respuesta_envio == NULL) {
		printf("Error alocando memoria\n");
		exit(1);
	}
	sprintf(respuesta_envio, "%s", mensaje_resp);
	enviar_a_cliente(respuesta_envio);
}

/*
	Verifica que la clave sea válida y la envía a auth_sevice
*/
void user_passwd(char* clave) {

	if( strlen(clave) < 5 || strlen(clave) > CLAVE_TAM) {
		enviar_a_cliente("Clave invalida");
		return;
	}

	char* cad_aux = malloc(strlen(clave));
	sprintf(cad_aux,"%s", clave);

	send_to_queue((long) PASSWORD_CHANGE, cad_aux);
	//free(cad_aux);
	recive_from_queue((long) PASSWORD_CHANGE_RESPONSE, 0);
	enviar_a_cliente("Clave cambiada con exito");

	return;
}

/*
	Verifica si el user quiere ver el listado de archivos o si quiere descargar.
	Si no es ninguna de estas opciones, notifica que es incorrecto.
*/
void file_command(char *opcion, char *argumento) {
	if( strcmp("ls", opcion) == 0 ){
		file_ls();
	}
	else if( strcmp("down", opcion) == 0 && strcmp(" ", argumento) != 0){
		file_down(argumento);
	}
	else {
		enviar_a_cliente(	" Opción Incorrecta\n"
							" Escriba: file [opcion] <argumento>\n"
							"	- ls : listado de archivos\n"
							"	- down <nombre_archivo> : descarga de archivo");
	}
}


/*
	Solicita al file_service la lista de imagenes
*/
void file_ls(){
	send_to_queue((long) FILES_REQUEST, "LISTA DE ARCHIVOS");
	mensaje_resp = recive_from_queue(FILES_RESPONSE, 0);

	envio_mensaje_aux();
}

/*
	Le envía al file_service el archivo que quiere descargar
*/
void file_down(char* archivo_nombre) {
	send_to_queue((long) DOWNLOAD_REQUEST, archivo_nombre);
	mensaje_resp = recive_from_queue(DOWNLOAD_RESPONSE, 0);

	envio_mensaje_aux();
	
}

/*
	Guarda la respuesta del buffer y la envía al cliente
*/
void envio_mensaje_aux(){
	char respuesta_envio[strlen(mensaje_resp)];
	sprintf(respuesta_envio, "%s", mensaje_resp);
	enviar_a_cliente(respuesta_envio);
}

/*
	Si el comando es incorrecto, envía un mensaje al cliente
	con los comandos válidos
*/
void unknown_command() {
	enviar_a_cliente(	"Comando no reconocido\n"
						"Comandos aceptados:\n"
						"	- user\n"
						"	- file\n"
						"	- exit");
}