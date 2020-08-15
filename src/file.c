#include "../include/file.h"

int32_t sockfd, sock_cli, puerto;
ssize_t n;							// hubo que declarar n como ssize_t para que no pierda información al usarse en send() y recv()
struct sockaddr_in serv_addr;
struct sockaddr_in client_addr;
char buffer[TAM], direccion[20];
uint32_t client_len;				//tamaño de la dirección del cliente
char* mensaje_resp;
Archivo* archivos[CANT_ARCHIVOS]; 

int32_t main(){
	
	//puerto = PUERTO_FILE.

	configurar_socket();

	Lista_de_archivos();

	escuchando();

	while(1){

		liste_user();
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
	serv_addr.sin_addr.s_addr = inet_addr(direccion);
	serv_addr.sin_port = htons( (uint16_t) puerto );
	if ( bind(sockfd, ( struct sockaddr *) &serv_addr, sizeof( serv_addr ) ) < 0 ) {
		printf("%sError de conexión\n",KRED);
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
    if(d = NULL){
    	printf("%sError leyendo archivo\n", KRED);
    	exit(1);
    }
    else{

        while ((dir = readdir(d)) != NULL){
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

        closedir(d);
    }
}

/*
	Verifica si se alocó la memoria correctamente
*/
void archivos_error(int32_t i){
	if(archivos[i] == NULL){
		printf("%sError alocando memoria\n", KRED);
		exit(1);
	}
}

/*
	Almacena los datos indice, nombre, formato, tamaño y hash
	en la estructura Archivo
*/
void guardar_datos(int32_t i,struct dirent *dir,char* path){
	archivos[i]->index=i;
	sprintf(archivos[i]->nombre,"%s",strtok(dir->d_name,"."));
	archivos[i]->nombre[strlen(archivos[i]->nombre)]='\0';
	sprintf(archivos[i]->formato,"%s",strtok(NULL,"."));
	archivos[i]->formato[strlen(archivos[i]->formato)]='\0';
	calc_size(i, path);	
	//sprintf(archivos[i]->hash,"%s",get_md5(path,0));

}

/*
	Abre el archivo del path seleccionado y calcula 
	cuantos bytes tiene, entonces lo asigna
*/
void calc_size(int32_t i,char* path){
	FILE* file = fopen(path, "rb"); //rb porque son archivos sin texto
	if(file != NULL){
		fseek(file,0,SEEK_END);
		ssize_t size = ftell(file);

		if(size < 0){
			// su ftell devuelve negativo, hay error
			// y asigna el valor máximo
			size = INT_MAX;
		}
		//al terminar, pasa el tamaño en Mb a la estructura
		archivos[i]->size=size/BYTES_TO_MB;

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
void liste_user(){

	//el server solicita la lista de archivos
	mensaje_resp = recive_from_queue((long)FILES_REQUEST,MSG_NOERROR|IPC_NOWAIT);
	if(errno != ENOMSG){
		files_request();
	}

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
		sprintf(size_aux,"%ld",archivos[i]->size);	//si no hago esto, hay un error de pointer a int
		size = size + strlen(indice) + strlen(archivos[i]->nombre) + strlen(guion) + 
					strlen(archivos[i]->formato) + strlen(guion) + 
					strlen(size_aux) + strlen(guion) +
					strlen(archivos[i]->hash);
		if(i < CANT_ARCHIVOS - 1){
			size = size + strlen(salto);
		}
	}

	char files[size];
	sprintf(files,"%s",encabezado);
	for(int32_t i = 0; i < CANT_ARCHIVOS; i++){
		char aux[strlen(files)];
		sprintf(aux,"%s",files);
		sprintf(files,"%s%d%s%s%s%s%s%ld%s%s%s",aux,archivos[i]->index,guion,
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
	int32_t flag = 0;
	int32_t indice_archivo = 0;
	//busca que archivo matchea el nombre con el buscado
	for(int32_t i; i < CANT_ARCHIVOS; i++){
		if(strcmp(archivos[i]->nombre,mensaje_resp)==0){
			flag = 1;
			indice_archivo = i;
		}
	}

	if(flag == 0){
		printf("%sNo se encontró el archivo\n", KYEL);
		send_to_queue((long)DOWNLOAD_RESPONSE,"descarga_no");
	}
	else{
		printf("%sArchivo encontrado\n", KGRN);
		send_to_queue((long)DOWNLOAD_RESPONSE,"descarga_si");

		enviar_archivo(indice_archivo);

	}
}

/*
	Abre un socket al cliente, envía el archivo al cliente y luego cierra conexión
*/
void enviar_archivo(int32_t indice_archivo){

	conectar_cliente();
	//enviar_a_cliente(indice_archivo);
	close(sock_cli);

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
	Envía datos hacia el cliente. Usado para respuestas de login y comandos
*/
void enviar_a_cliente(char* mensaje) {
	n = send( sock_cli, mensaje, strlen(mensaje), 0 );
	if ( n < 0 ) {
		printf("error enviando a cliente\n" );
	  	exit( 1 );
	}
}