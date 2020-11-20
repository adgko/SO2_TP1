#include "../include/file.h"



int32_t main(){

	configurar_socket();	// configura y abre el socket para transmitir 

	Lista_de_archivos();	// levanta los archivos en una base de datos

	escuchando();			// espera que se conecte un socket

	while(1){

		listen_user();		// lee la cola de mensajes para saber si se envió ls o down
		sleep(TIME_SLEEP);
	}

	exit(0);
}

/*
	Configura el socket con el que se enviará la imagen
*/
void configurar_socket() {
	sockfd = socket( AF_INET, SOCK_STREAM, 0);
	memset( (char *) &serv_addr, 0, sizeof(serv_addr) );

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons( (uint16_t) puerto_files);
	if ( bind(sockfd, ( struct sockaddr *) &serv_addr, sizeof( serv_addr ) ) < 0 ) {
		perror( "conexion" );
		exit(1);
	}
}

/*
	Lista los archivos de un directorio dado y
	los almacena en un buffer para ser leídos por el programa
*/
void Lista_de_archivos(){

	DIR *d;
    struct dirent *dir;
    int32_t i = 0;

    d = opendir(IMAGES_PATH);
    if(d != NULL){			
    	while ((dir = readdir(d)) != NULL){
        	if(dir->d_name[0] != '.'){
           		//asignar espacio en el arreglo
            	archivos[i]=malloc(sizeof(Archivo));
            	archivos_error(i);

            	//crea el path con el path que tiene y el nombre del archivo
            	char path[strlen(IMAGES_PATH) + strlen(dir->d_name) + 1];
				sprintf(path, "%s%s", IMAGES_PATH, dir->d_name);
				path[strlen(path)] = '\0';

				guardar_datos(i,dir,path);
				i++;


			}

    	}
    }
    else{
    	printf("%sError leyendo directorio%s\n", KRED,KNRM);
    	exit(1);
        }

        closedir(d);
}

/*
	Verifica si se alocó la memoria correctamente
*/
void archivos_error(int32_t i){
	if(archivos[i] == NULL){
		printf("%sError alocando memoria%s\n", KRED,KNRM);
		exit(1);
	}
}

/*
	Almacena los datos indice, nombre, formato, tamaño y hash
	en la estructura Archivo
*/
void guardar_datos(int32_t i,struct dirent *dir,char* path){

	archivos[i]->index=i;
	printf("%d\n", archivos[i]->index);										// indice
	sprintf(archivos[i]->nombre,"%s",strtok(dir->d_name,"."));	// nombre
	archivos[i]->nombre[strlen(archivos[i]->nombre)]='\0';
	printf("%s\n",archivos[i]->nombre );
	sprintf(archivos[i]->formato,"%s",strtok(NULL,"."));		// formato
	archivos[i]->formato[strlen(archivos[i]->formato)]='\0';
	printf("%s\n", archivos[i]->formato);
	calc_size(i, path);			
	printf("%f\n", archivos[i]->size);								// tamaño
	char *md5 = get_MD5(path,0);								// md5
	sprintf(archivos[i]->hash,"%s",md5);
	printf("%s\n", archivos[i]->hash);


}

/*
	Abre el archivo del path seleccionado y calcula 
	cuantos bytes tiene, entonces lo asigna
*/
void calc_size(int32_t i,char* path){

	FILE* file = fopen(path, "rb"); //rb porque son archivos sin texto
	if(file != NULL){
		fseek(file,0L,SEEK_END);
		long int size = ftell(file);

		if(size < 0){
			// su ftell devuelve negativo, hay error
			// y asigna el valor máximo
			size = INT_MAX;
		}
		//al terminar, pasa el tamaño en Mb a la estructura
		float aux = (float)size/BYTES_TO_MB;
		archivos[i]->size=aux;

		fclose(file);
	}
}

/*
	Abre conexión para que se conecte un clietne
*/
void escuchando(){
	listen(sockfd, 1);
	client_len = sizeof(client_addr);
}

/*
	Escucha esperando mensajes. Estos están seteados en IPC_NOWAIT
	para que si no hay mensajes en la cola, errno=ENOMSG y sigue la ejecución
	preguntando por el siguiente tipo de mensaje
*/
void listen_user(){

	//el server solicita la lista de archivos
	mensaje_resp = recive_from_queue((long)FILES_REQUEST,MSG_NOERROR|IPC_NOWAIT);
	if(errno != ENOMSG){
		files_request();
	}

	//el server solicita un archivo para que envíe al cliente
	mensaje_resp = recive_from_queue((long)DOWNLOAD_REQUEST,MSG_NOERROR|IPC_NOWAIT);
	if(errno != ENOMSG){
		download_request();
		//printf("%sFunción fuera de servicio %s\n",KYEL,KNRM );
		//send_to_queue((long)DOWNLOAD_RESPONSE,"Función fuera de servicio");
	}

}
/*
	Va almacenando en un buffer todo el contenido del arreglo archivos,
	los ordena en una tabla y luego los envía a la cola como respuesta
*/
void files_request(){

	char* encabezado = "[Indice] - [Nombre] - [Formato] - [Tamaño (MB)] - [Hash]\n";
	size_t size = strlen(encabezado);
	char* guion = " - ";
	char* salto = "\n";
	char indice[3] = "";
	char size_aux[ARCHIVO_NAME_SIZE];		//se usa para guardar size por problema de conversion
	for(int32_t i = 0; i < CANT_ARCHIVOS; i++) {
		sprintf(size_aux,"%f",archivos[i]->size);	//si no hago esto, hay un error de pointer a int
		size = size + strlen(indice) + strlen(archivos[i]->nombre) + strlen(guion) + 
					strlen(archivos[i]->formato) + strlen(guion) + 
					strlen(size_aux) + strlen(guion) +
					strlen(archivos[i]->hash);
		if(i < CANT_ARCHIVOS - 1){
			size = size + strlen(salto);
		}
	}

	char files[size];
	if(files == NULL){
		printf("%sError alocando memoria%s\n",KRED,KNRM);
		exit(1);
	}
	sprintf(files,"%s",encabezado);
	
	for(int32_t i = 0; i < CANT_ARCHIVOS; i++){
		char aux[strlen(files)];
		sprintf(aux,"%s",files);
		sprintf(files,"%s%d%s%s%s%s%s%f%s%s%s",aux,archivos[i]->index,guion,
				archivos[i]->nombre,guion,archivos[i]->formato,guion,
				archivos[i]->size,guion,archivos[i]->hash,salto);
	}
	send_to_queue((long)FILES_RESPONSE, files);
}

/*
	Busca el archivo solicitado, avisa si lo encontró o no, y en caso afirmativo
	envía dicho archivo
*/
void download_request(){
/*
	int32_t flag = 0;
	int32_t indice_archivo = 0;
	//busca que archivo matchea el nombre con el buscado
	for(int32_t i=0; i < CANT_ARCHIVOS; i++){
		char aux[strlen(archivos[i]->nombre)+strlen(".")+strlen(archivos[i]->formato)];		//se cambió comparación con nombre por esto, 
		sprintf(aux,"%s.%s",archivos[i]->nombre,archivos[i]->formato);						// porque no podía encontrar el archivo sino
		if(strcmp(aux,mensaje_resp)==0){
			flag = 1;
			indice_archivo = i;
		}
	}

	if(flag == 0){
		printf("%sNo se encontró el archivo%s\n", KYEL,KNRM);
		send_to_queue((long)DOWNLOAD_RESPONSE,"descarga_no");
	}
	else{
		printf("%sArchivo encontrado%s\n", KGRN,KNRM);
		send_to_queue((long)DOWNLOAD_RESPONSE,"descarga_si");

		conectar_enviar(indice_archivo);

	}*/
	printf("Probando download 1\n");
	char img[TAM];
    long size = 0;
	//char *tok = strtok(buffer, " ");

	printf("Probando download 2\n");

	strcpy(img, IMAGES_PATH); /* Nombre de la imagen */
  	//strcat(img, "/");
  	strcat(img, "bionicpup32.iso");

  	printf("%s\n",img );

  	printf("Probando download 3\n");

  	//tok = strtok(NULL, " "); /* Dispositivo a escribir */
  	//char usb[TAM];
  	//strcpy(usb,tok);

  	//printf("%s\n", usb);

  	printf("Probando download 4\n");

  	FILE *imgn;
  	imgn = fopen(img, "r");

  	if(imgn == NULL)
  	{
    	perror("file");
    	//strcpy(buffer, "La imagen no existe.\n");
    	//msgsnd(msqid, &buf, sizeof(buf.msg), 0);
    	send_to_queue((long)DOWNLOAD_RESPONSE,"descarga_no");
    	return;
  	}

  	printf("Probando download 5\n");

  	fseek(imgn, 0, SEEK_END); /* Calcula el tamaño de la imagen */
  	size = ftell(imgn);
  	fclose(imgn);

  	printf("Probando download 6\n");

  	char size_s[TAM] = "";
  	sprintf(size_s, "%ld", size);

  	printf("%s\n",size_s );

  	printf("Probando download 7\n");

  	char *md5s = get_MD5(img, 0);

  	sprintf(buffer, "Download %s %s", size_s, md5s);
  	printf("%s\n",buffer);

  	printf("Probando download 8\n");

  	//msgsnd(msqid, &buf, sizeof(buf.msg), 0);
  	send_to_queue((long)DOWNLOAD_RESPONSE,buffer);

  	printf("Probando download 9\n");

  	//int fifd = -1;
    conectar_cliente();

    printf("Probando download 10\n");

    send_image(img, &size);

    printf("Probando download 11\n");


}

/*
void start_listening(int sockfd, char *ip)
{
  //sockfd = create_svsocket(ip, port_fi);
	conectar_cliente();

  struct sockaddr_in cl_addr;
  uint cl_len;
  char cl_ip[STR_LEN];

  printf("Esuchando en puerto %d...\n", puerto_files);

  listen(sock_cli, 1);
  cl_len = sizeof(cl_addr);

  //int newfd;
  //newfd = accept(sockfd, (struct sockaddr *) &cl_addr, &cl_len);
  //check_error(newfd);

  //inet_ntop(AF_INET, &(cl_addr.sin_addr), cl_ip, INET_ADDRSTRLEN);
  printf("Conexión aceptada a %s\n", cl_ip);

  close(sock_cli);
  //return newfd;
}*/

void send_image(char* img, long *size)
{
  int32_t imgfd;

  imgfd = open(img, O_RDONLY);
  //check_error(imgfd);
  if(imgfd == -1)
  		{
    		perror("error");
    		exit(EXIT_FAILURE);
  		}

  printf("%s\n", "  FS: Sending file...");

  size_t to_send = (size_t) size;
  ssize_t sent;
  off_t offset = 0;

  while(((sent = sendfile(sock_cli, imgfd, &offset, to_send)) > 0)
        && (to_send > 0))
  {
    to_send -= (size_t) sent;
  }

  printf("  FS: %lu %s\n", *size, "sent.");

  close(imgfd);
}

/*
	Abre un socket al cliente, envía el archivo al cliente y luego cierra conexión
*/
void conectar_enviar(int32_t indice_archivo){

	conectar_cliente();
	enviar_archivo(indice_archivo);
	//close(sock_cli);

}

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
	
*/
void enviar_archivo(int32_t i){

	//guarda el nombre del archivo para enviarlo al cliente y usarlo en el path
	sprintf(nombre_archivo,"%s",mensaje_resp);
	nombre_archivo[strlen(nombre_archivo)] = '\0';

	//crea el path donde va a buscar dicho archivo
	char* path = malloc(strlen(IMAGES_PATH) + strlen(nombre_archivo));
	verificar_path(path);
	memset(path,0,strlen(path));
	sprintf(path,"%s%s",IMAGES_PATH,nombre_archivo);
	path[strlen(path)] = '\0';


	char tamanio[ARCHIVO_NAME_SIZE];
	memset(tamanio,0,ARCHIVO_NAME_SIZE);
	sprintf(tamanio,"%f",archivos[i]->size);
	tamanio[strlen(tamanio)] = '\0';

	char* cad_aux = malloc(strlen(nombre_archivo) + strlen(tamanio));
    if(cad_aux == NULL) {
		perror("error de alocación\n");
		exit(1);
	}
	sprintf(cad_aux, "%s %s", nombre_archivo, tamanio);
	printf("%s\n",cad_aux );

	enviar_a_cliente(cad_aux);

	//confirmacion_cliente();

	enviar_a_cliente_archivo(path);

}

/*
	Envía datos hacia el cliente. Usado para respuestas de login y comandos
*/
void enviar_a_cliente(char* mensaje) {
	n = send( sock_cli, mensaje, strlen(mensaje), 0 );
	if ( n < 0 ) {
		printf("%sError enviando a cliente%s\n",KRED,KNRM );
	  	exit( 1 );
	}
}

void confirmacion_cliente(){
	memset( buffer,0,TAM);
	n = recv( sock_cli,buffer,TAM,0);
	if ( n < 0 ) {
	  perror( "error de recepción\n" );
	  exit(1);
	}

	if(!strcmp(buffer,"ok")==0){
		printf("%sError en la confirmación %s\n",KRED,KNRM );
		shutdown(sockfd,SHUT_RDWR);
		exit(1);
	}

	printf("%sDatos recibidos %s\n",KGRN,KNRM );
	confirm_flag = 1;
}

/*
	Revisa si la memoria se alocó correctamente 
*/
void verificar_path(char* path){
	if(path == NULL){
		printf("%sError alocando memoria%s\n", KRED,KNRM);
		exit(1);
	}
}


void enviar_a_cliente_archivo(char* path){

	FILE* file = fopen(path, "rb"); //rb porque son archivos sin texto
	if(file != NULL){
		
		while(fread(buffer, sizeof(char),sizeof(buffer), file) > 0){
			n = send(sock_cli, buffer, sizeof(buffer),0);
			if(n < 0){
				printf("%sError enviando al cliente%s\n", KYEL,KNRM);
			}
			printf("%s\n",buffer );
			printf("%sEnviando Archivos%s\n",KGRN,KNRM );
			fclose(file);
		}
		
	}
	else{
		printf("%sError abriendo archivo%s\n",KRED,KNRM );
		exit(1);
	}
	free(path);
	shutdown(sockfd,SHUT_RDWR);
	
}

