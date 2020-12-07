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
			printf("%sBase de Datos Completada%s\n",KGRN,KNRM );

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
	//printf("%d\n", archivos[i]->index);							// indice
	sprintf(archivos[i]->nombre,"%s",strtok(dir->d_name,"."));		// nombre
	archivos[i]->nombre[strlen(archivos[i]->nombre)]='\0';
	//printf("%s\n",archivos[i]->nombre );
	sprintf(archivos[i]->formato,"%s",strtok(NULL,"."));			// formato
	archivos[i]->formato[strlen(archivos[i]->formato)]='\0';
	//printf("%s\n", archivos[i]->formato);
	calc_size(i, path);			
	//printf("%f\n", archivos[i]->size);							// tamaño
	char *md5 = get_MD5(path,0);									// md5
	sprintf(archivos[i]->hash,"%s",md5);
	//printf("%s\n", archivos[i]->hash);
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

	char img[TAM];
    long size = 0;

    char aux[strlen(mensaje_resp)];
    sprintf(aux, "%s", mensaje_resp);

    printf("%sSe solicitó el archivo: %s%s\n", KBLU,aux,KNRM);

	sprintf(img,"%s%s",IMAGES_PATH,aux);		//Path de la imagen
				
  	FILE *imgn;
  	imgn = fopen(img, "r");
  	if(imgn == NULL)
  	{
    	perror("file");
    	send_to_queue((long)DOWNLOAD_RESPONSE,"descarga_no");
    	return;
  	}

  	// Calcula el tamaño de la imagen
  	fseek(imgn, 0, SEEK_END); 							
  	size = ftell(imgn);
  	fclose(imgn);

  	// guarda tamaño para enviar
  	char size_s[TAM] = "";
  	sprintf(size_s, "%ld", size);						

  	char *md5s = get_MD5(img, 0);

  	// guarda en buffer el tamaño y hash para enviar
  	sprintf(buffer, "Download %s %s", size_s, md5s);	

  	send_to_queue((long)DOWNLOAD_RESPONSE,buffer);

    conectar_cliente();

    //Envía la imagen por socket
    enviar_imagen(img, &size);

}


/*
	Abre la imagen, si no la encuentra emite error
	Calcula su tamaño y hasta que no envíe ese tamaño no para de enviar
*/
void enviar_imagen(char* img, long *size){

  int32_t imgfd;
  imgfd = open(img, O_RDONLY);
  if(imgfd == -1)
  		{
    		perror("error");
    		exit(EXIT_FAILURE);
  		}

  printf("%s Enviando imagen a grabar %s\n",KBLU,KNRM);

  size_t to_send = (size_t) size;
  ssize_t sent;
  off_t offset = 0;

  while(((sent = sendfile(sock_cli, imgfd, &offset, to_send)) > 0)
        && (to_send > 0))
  {
    to_send -= (size_t) sent;
  }

  printf(" %sSe envió %lu %s\n", KGRN, *size, KNRM);

  close(imgfd);
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

