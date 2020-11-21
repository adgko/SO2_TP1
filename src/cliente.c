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
	Configura tanto el socket para conectarse con el server primario, como también para archivos.
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
void salida() {								
	
	printf("%sVuelvas prontoss%s \n",KBLU,KNRM);
	fflush(stdout);
	if(auth_flag == 1)
		enviar_a_socket(sockfd, "exit");		// avisa al server que se va
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
		salida();
	}
}

/*
	Solicita comandos, si es exit, cerrará la sesión, sino los enviará al server para que se encargue
*/
void comandos(){
	while(1){
		memset(buffer, 0, TAM);			// con esto limpio el buffer del comando anterior
		printf("$");
		fgets(buffer,TAM,stdin); 			// pido comando
		if(buffer[0] != '\n'){				//si en el buffer no hay una nueva linea, envía
			if(strcmp(buffer, "exit\n") == 0){	//si se escribe "exit", cierra sesión y cierra el programa
				salida();
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

		else if(strstr(buffer,"Download") != NULL){		// Si alguna parde del mensaje dice "Downdload", entra acá	
        												// Esto se debe a que el server envía Download tamaño y hash MD5
        	conect_to_files();							

        	printf("%sEscribiendo USB %s%s\n", KGRN,PATH_USB,KNRM);

        	escribir_usb();

        	close(sockfil);
        }

		else {
			printf("%s\n", buffer);
			fflush(stdout);
		}		
	}
}

/*
	Recibe la respuesta del Socket server
*/
void recibir_respuesta(int32_t socket) {
	memset( buffer,0,TAM);
	n = recv( socket,buffer,TAM,0);
	if ( n < 0 ) {
	  perror( "error de recepción\n" );
	  exit(1);
	}
}

/*
	Si el archivo es enviado, será escrito en un USB
*/
void escribir_usb()
{
  char *tok = strtok(buffer, " ");
  tok = strtok(NULL, " ");
  size_t size = 0;
  size_t md5_aux;

  /*
	Almaceno el tamaño que recibo para poder calcular el MD5
  */	
  sscanf(tok, "%lud", &size);
  md5_aux = size;

  /*
	Almaceno el MD5 que me llegó para compararlo luego
  */
  char md5_recv[MD5_DIGEST_LENGTH*2+1];
  tok = strtok(NULL, " ");
  sprintf(md5_recv, "%s", tok);

  FILE *usb;
  usb = fopen(PATH_USB, "wb");

  if(usb == NULL)
    {
      perror("open usb");
      exit(EXIT_FAILURE);
    }

  	// lo que recibe, se escribe en un archivo en el pendrive
  	/*
		Se emplea un bucle do while para asegurar que se cumple al menos una vez el ciclo
		y luego comprueba en cada iteración
  	*/
  do
    {
      n = recv(sockfil, buffer, TAM, 0);
      if ( n < 0 ) {
	  	perror( "error de recepción\n" );
	  	exit(1);
	  }

      fwrite(buffer, sizeof(char), (size_t) n, usb);
      size -= (size_t) n;
    }
  while(size != 0);

  sync();			// se emplea sync porque sino se pisa el buffer cuando envío más comandos
  fclose(usb);

  printf("%sEscritura de USB terminada %s\n",KGRN,KNRM);

  char *md5 = get_MD5(PATH_USB, md5_aux);

  /*
	si los MD5 matchean, puedo marcar que fue exitosa y mostrar MBR
	si no, se dirá que falló
  */
  if(!strcmp(md5_recv, md5))
    {
      printf("Escritura exitosa.\n");
      show_MBR(PATH_USB);
    }
  else
    printf("Escritura fallida.\n");
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
	if ( connect( sockfil, (struct sockaddr *)&serv_addr_file, sizeof(serv_addr_file ) ) < 0 ) {
		perror( "conexion" );
		exit( 1 );
	}
}

