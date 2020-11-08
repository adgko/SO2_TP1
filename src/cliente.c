#include "../include/cliente.h"

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

	conect_to_server();			// configura el socket y se conecta con el servidor

	signal_handler();			//configuro el handler para que apunte a salida
	
	while(1) {

			/*
				Si auth_flag es 0, no hay usuarios autenticados, por lo que pedirá credenciales hasta que encuentre alguno o se bloquee
			*/
			if(auth_flag == 0){
				rta = login();
				validar_rta();
			}

			/*
				Si auth_flag es 1, un usario se logueó y puede enviar comandos
			*/
			else{
				comandos();
				leer_server();
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
	serv_addr_file.sin_family = AF_INET;
	bcopy( (char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, (size_t)server->h_length );		// hay que castear server->h_length para que fucnione en bcopy
	bcopy( (char *)server->h_addr, (char *)&serv_addr_file.sin_addr.s_addr, (size_t)server->h_length );
	serv_addr.sin_port = htons( (uint16_t) puerto_connect );
	serv_addr_file.sin_port = htons( (uint16_t) puerto_files );
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

	printf("%sIngrese su usuario:%s\n",KMAG,KNRM);
    fgets(usuario,TAM,stdin);
    printf("%sIngrese su contraseña:%s\n",KMAG,KNRM);
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
	recibir_respuesta(sockfd);

	return validar_login();
	
	
}

/*
	si el server escribe un 1 en el buffer, se logueo correctamente. 
	Si escribe un 0, falló y pide nuevamente las credenciales
	Si escribe cualquier otra cosa, el usuario está bloqueado.
*/
int32_t validar_login(){
	if( strcmp(buffer,"1") == 0 ){						
		return 1;
	}
	else if( strcmp(buffer,"0") == 0 ){
		return 0;
	}
	else{
		return 2;
	}
}

/*
	Corrobora la respuesta, evaluando si el usuario está logueado, bloqueado o incorrecto
*/
void validar_rta(){
	if(rta == 0)
		printf("%scredenciales erroneas, vuelva a intentar%s\n",KYEL,KNRM);
	else if(rta == 1) {
		printf("%sCONECTADO%s\n",KGRN,KNRM);
		auth_flag = 1;
	}
	else if(rta == 2) {
		printf("\n%sUSUARIO BLOQUEADO%s\n\n",KRED,KNRM);
		salida(0);
	}
}

void comandos(){
	while(1){
		memset(buffer, 0, TAM);			// con esto limpio el buffer del comando anterior
		printf("$");
		fgets(buffer,TAM,stdin); 			// pido comando
		if(buffer[0] != '\n'){				//si en el buffer no hay una nueva linea, envía
			if(strcmp(buffer, "exit\n") == 0){	//si se escribe "exit", cierra sesión y cierra el programa
				salida(1);
			}

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

/*
	reacciona a la respuesta del server
	si no se puede descargar avisa
	si se puede descargar llama a descargar()
	en otro caso imprime lo que le llega (user ls, user passwd, file ls)
*/
void leer_server(){
	if(buffer[0] != '\0'){		//por si el mensaje es nulo
		printf("recibiendo respuesta\n");
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
void recibir_respuesta(int32_t socket) {
	memset( buffer,0,TAM);
	n = recv( socket,buffer,TAM,0);
	if ( n < 0 ) {
	  perror( "error de recepción\n" );
	  exit(1);
	}
}

void descargar(){
	
	printf("Probando download 1\n");
	//long int size;
	char buffer_aux[MD5_DIGEST_LENGTH*2];
	memset(buffer_aux,0,MD5_DIGEST_LENGTH*2);

	printf("Probando download 2\n");

	conect_to_files();
	memset( buffer,0,TAM);	
	recibir_respuesta(sockfil);	//recibe el nombre del archivo
	printf("%s\n", buffer);
	printf("Probando download 3\n");
	//printf("\n");
/*
	char archivo = strtok(buffer, " ");
	//char archivo[strlen(buffer)];
	//sprintf(archivo, "%s", buffer);
	archivo[strlen(archivo)] = '\0';
	memset( buffer,0,TAM);
	printf("%s\n", archivo );

	//fflush(stdout);
	printf("Probando download 4\n");

	//recibir_respuesta(sockfil);	//recibe taaño de archivo
	printf("%s\n", buffer);
	//fflush(stdout);
	printf("Probando download 5\n");

	char tamanio = strtok(buffer, " ");
	//char tamanio[strlen(buffer)];
	//sprintf(tamanio,"%s",buffer);
	tamanio[strlen(tamanio)] = '\0';
	printf("%s\n",tamanio );
	//fflush(stdout);
	printf("Probando download 6\n");
*/

	//recibir_datos();
	printf("Probando download 4\n");
	char* aux_data;

	aux_data = strtok(buffer, " ");
	char archivo[strlen(aux_data)];
	sprintf(archivo, "%s", aux_data);

	printf("Probando download 5\n");

	aux_data = strtok(buffer, " ");
	char tamanio[strlen(aux_data)];
	sprintf(tamanio, "%s", aux_data);

	printf("Probando download 6\n");

	confirmar_files();

	printf("Probando download 7\n");

	printf("%sDescargando archivo%s\n",KGRN,KNRM );
	FILE* file = fopen(PATH_USB, "wb+");	//escribe archivo y si no hay, lo crea
	if(file != NULL){
		ssize_t download=0;		//me sirve para saber cuanto descargo
		char* aux_down = malloc(sizeof(download));
		while((n=recv(sockfil,buffer,sizeof(buffer),0))>0){
			sprintf(aux_down,"%ld",download);
			if(!(strcmp(aux_down,tamanio))){		//si matchea el tamaño con lo que descargo, freno
				break;
			}
			fwrite(buffer,sizeof(char),(size_t)n,file);
			download+=n;	//a medida que lee, aumento el tamaño para sacar el MD5
		}
		//size = ftell(file);
		fclose(file);
		shutdown(sockfd,SHUT_RDWR);
		close(sockfil);

		float aux=(float)download/BYTES_TO_MB;
		printf("%sDescarga terminada: %f MB%s\n",KBLU,aux,KNRM);
		fflush(stdout);
	}
	else{
		printf("%sError descargando archivo%s\n",KRED,KNRM);
		shutdown(sockfd,SHUT_RDWR);
		exit(1);
	}

	//get_MD5(PATH_USB,buffer_aux);
	//printf("%sHash del archivo: %s%s\n",KBLU,buffer_aux,KNRM);
	

/*
	ssize_t size;
	if(!strcmp(buffer,"connOk")){
		printf("conectando\n");
		//se obtine socket y se establece conexion
		sockfil = socket( AF_INET, SOCK_STREAM, 0 );
		if ( sockfil < 0 ) {
			perror( "ERROR apertura de socket" );
			exit( 1 );
		}

		if ( connect( sockfil, (struct sockaddr *)&serv_addr_file, sizeof(serv_addr_file ) ) < 0 ) {
			perror( "conexion" );
			exit( 1 );
		}
	}else{
		return;
	}

	memset( buffer, 0, TAM );

	printf("iniciando descarga\n");
	//se abre el archivo destino con modo wb para la escritura
	FILE* file = fopen( PATH_USB, "wb" );

  	if(file != NULL) {
		ssize_t readed=0;

		while((size=recv(sockfil, buffer, sizeof(buffer), 0))> 0 ){
			if(!(strcmp(buffer,"EOF")))
				break;
			fwrite(buffer, sizeof(char), (size_t) size, file);
			readed+=size;
		}
		//se obtienen los datos una vez finalizada la descarga
		//char filename[ARCHIVO_NAME_SIZE]=PATH_USB;
		//char auxBuffer[ARCHIVO_NAME_SIZE];
		printf("Descarga terminada: %ld Bytes\n",readed);
		//get_MD5(filename,auxBuffer);
		//sprintf("Hash MD5 <%s>\n",auxBuffer);
		fclose(file);
		//getMbr(auxBuffer);
		//printf("Datos de la tabla de particiones MBR:\n%s",auxBuffer);
		//se cierra la conexion
		shutdown(sockfil,SHUT_RDWR);
	}else {
      	perror("error al descargar archivo\n");
		exit(1);
  	}*/

}

/*
	Es lo mismo que connect_to_server, pero para file, 
	así le pasa el archivo que quiere descargar por otro socket distinto
	al que usa para enviar comandos
*/
void conect_to_files(){
	sockfil = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sockfil < 0 ) {
		perror( "ERROR apertura de socket" );
		exit( 1 );
	}
	//memset( (char *) &serv_addr_file, '0', sizeof(serv_addr) );
	//serv_addr_file.sin_family = AF_INET;
	//bcopy( (char *)server->h_addr, (char *)&serv_addr_file.sin_addr.s_addr, (size_t)server->h_length );		// hay que castear server->h_length para que fucnione en bcopy
	//serv_addr_file.sin_port = htons( (uint16_t)puerto2 );
	if ( connect( sockfil, (struct sockaddr *)&serv_addr_file, sizeof(serv_addr_file ) ) < 0 ) {
		perror( "conexion" );
		exit( 1 );
	}
}

void confirmar_files(){					
	n = send( sockfil, "ok", strlen("ok"), 0 );
	if ( n < 0 ) {
	  perror( "error de envío\n");
	  exit( 1 );
	}
}
