#include "../include/auth.h"

Usuario* usuarios[CANTIDAD_USUARIOS];
char* mensaje;
char user[USUARIO_TAM];
char user_aux[USUARIO_TAM];

int32_t main(){

	printf("%sIniciando Auth Service%s\n", KBLU,KNRM );

	leer_bd();

	while(1){		//se queda esperando que en la cola haya algo para él
		listen_user();
	}
	exit(0);
}

/*
	Abre un archivo txt, guarda todas las líneas y las almacena como
	campos de la estructura Usuario
*/
void leer_bd() {


	char lineas[CANTIDAD_USUARIOS * USUARIO_CAMPOS][RENGLON];
	int32_t renglones = 0;
	
	FILE *file;
	file = fopen(base_datos_usuarios, "r"); // para leer

	if ( file != NULL ) {
      	char line[RENGLON];
		int32_t index = 0;
      	while ( fgets( line, RENGLON, file ) != NULL ) {
				sprintf(lineas[index], "%s", line);
				index++;
				renglones++;
      	}

      	
 	}
	else{
		printf("%sError al Leer la Base de Datos%s\n", KRED,KNRM);
		exit(1);
	}
	fclose(file);
	/*
		Por cada usuario y por cada campo de todo el grupo, se guarda 
		los datos obtenidos del documento
	*/
	for(int32_t i = 0,k = 0; i < CANTIDAD_USUARIOS && k < renglones;i++) {
		usuarios[i] = malloc(sizeof(Usuario));
		if(usuarios[i] == NULL) {
			printf("%sError alocando memoria en usuarios%s\n",KRED,KNRM);
			exit(1);
		}
		sprintf(usuarios[i]->usuario, "%s", lineas[k++]);
		usuarios[i]->usuario[strlen(usuarios[i]->usuario) - 1] = '\0';
		printf("%s\n", usuarios[i]->usuario);
		sprintf(usuarios[i]->password, "%s", lineas[k++]);
		usuarios[i]->password[strlen(usuarios[i]->password) - 1] = '\0';
		printf("%s\n", usuarios[i]->password);
		sprintf(usuarios[i]->intentos, "%s", lineas[k++]);
		usuarios[i]->intentos[strlen(usuarios[i]->intentos) - 1] = '\0';
		printf("%s\n", usuarios[i]->intentos);
		sprintf(usuarios[i]->ultima_conexion, "%s", lineas[k++]);
		usuarios[i]->ultima_conexion[strlen(usuarios[i]->ultima_conexion) - 1] = '\0';
		printf("%s\n", usuarios[i]->ultima_conexion);
		
	}
}

/*
	recorre el arreglo archivo, limpiando la memoria de sus campos
	para que no quede nada de corridas anteriores
*/
void vaciar_archivos(){
	for(int32_t i=0;i<CANTIDAD_USUARIOS;i++){
		memset(usuarios[i]->usuario,0,sizeof(usuarios[i]->usuario));
		memset(usuarios[i]->password,0,sizeof(usuarios[i]->password));
		memset(usuarios[i]->intentos,0,sizeof(usuarios[i]->intentos));
		memset(usuarios[i]->ultima_conexion,0,sizeof(usuarios[i]->ultima_conexion));
	}
}

/*
	Escucha esperando mensajes. Estos están seteados en IPC_NOWAIT
	para que si no hay mensajes en la cola, errno=ENOMSG y sigue la ejecución
	preguntando por el siguiente tipo de mensaje
*/
void listen_user(){
	// el server solicita que loguee un user
	mensaje = recive_from_queue((long)LOGIN_REQUEST,MSG_NOERROR | IPC_NOWAIT);
	if(errno != ENOMSG){
		login_request();
	}

	// el server solicita la lista de usuarios, si están bloqueados
	// y su última conexión
	mensaje = recive_from_queue((long)NAMES_REQUEST,MSG_NOERROR | IPC_NOWAIT);
	if(errno != ENOMSG){
		names_request();
	}

	// el server solicita un cambio de contraseña
	mensaje = recive_from_queue((long)PASSWORD_CHANGE,MSG_NOERROR | IPC_NOWAIT);
	if(errno != ENOMSG){
		password_change();
	}

	//actualiza el archivo antes de preguntar de nuevo
	//actualizar_bd();
}

/*
	guarda las credenciales y las usa para logearse 
*/
void login_request(){
	
		char credenciales[strlen(mensaje)];
		if(credenciales == NULL){
			printf("%sError alocando memoria%s\n",KRED,KNRM);
			exit(1);
		}
		sprintf(credenciales,"%s",mensaje);

		printf("%srevisando login_request%s\n",KCYN,KNRM);

		int32_t log = login(credenciales);

		printf("%s\n",user );

		int32_t rta = verificar_log(log);

		char aux[3] = "";
		sprintf(aux,"%d",rta);
		
		
		send_to_queue((long)LOGIN_RESPONSE,aux);
		
}

/*
	loguea con las credenciales del usuario
*/
int32_t login(char* credenciales){
	
	char* login;

	login = strtok(credenciales, "\n");
	char usuario[strlen(login)];
	sprintf(usuario, "%s", login);

	printf("%sLOGIN REQUEST %s%s\n",KCYN,usuario,KNRM );

	login = strtok(NULL, "\n");
	char password[strlen(login)];
	sprintf(password,"%s",login);

	for(int32_t i = 0; i < CANTIDAD_USUARIOS; i++) {
		if( strcmp(usuario, usuarios[i]->usuario) == 0 ) {
			sprintf(user_aux,"%s",usuario);
			if(get_bloqueado()){
				if( strcmp(password, usuarios[i]->password) == 0 ) {
					set_ultima_conexion();
					sprintf(usuarios[i]->intentos,"%s", "0");		//al loguear, setea los intentos en 0
					sprintf(user,"%s",usuarios[i]->usuario);
					actualizar_bd();
					return 1;
				}
				else{
					int32_t aux = set_intentos(i);
					sprintf(usuarios[i]->intentos,"%d", aux);
					printf("%sfallo el password%s\n",KYEL,KNRM);
					printf("%sintentos: %s%s\n", KYEL,usuarios[i]->intentos,KNRM);
					actualizar_bd();
					return 0;
				}
			}
			else{
				return 2;   // usuario bloqueado
			}
		}
	}
	return 0;
}

/*
	Valida si está bloqueado
	Devuelve 1 si tiene intentos disponibles
	Devuelve 0 si no los tiene
*/
int32_t get_bloqueado(){
	for(int32_t i=0; i<CANTIDAD_USUARIOS; i++){
		if(strcmp(user_aux,usuarios[i]->usuario) == 0){
			int32_t intentos = atoi(usuarios[i]->intentos);
			if(intentos < 3)
				return 1;
			else
				return 0;
		}
	}
	return 0;
}

/*
	Calcula el tiempo actual y lo guarda en su campo correspondiente
*/
void set_ultima_conexion(){
	time_t rawtime = time(NULL);
    if (rawtime == -1) {
        perror("tiempo fallido");
        return;
    }

    struct tm *ptm = localtime(&rawtime);
    if (ptm == NULL) {
        perror("tiempo local fallido");
        return;
    }

    char aux[LAST_CONECTION_SIZE];
    sprintf(aux,"%d-%02d-%02d %02d:%02d",ptm->tm_year + 1900,
	 ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, 
           ptm->tm_min);

    printf("%s\n",aux );

    for(int32_t i=0; i<CANTIDAD_USUARIOS; i++){
		if(strcmp(user_aux,usuarios[i]->usuario) == 0){
			sprintf(usuarios[i]->ultima_conexion,"%s",aux);
			usuarios[i]->ultima_conexion[strlen(usuarios[i]->ultima_conexion) - 1] = '\0';
		}
	}
	return;
}

/*
	Recupera cuantos intentos tiene el usuario
	y lo aumenta en 1
*/
int32_t set_intentos(int32_t i){
	int32_t aux = 0;
	aux = atoi(usuarios[i]->intentos);
	aux++;
	return aux;
}
/*
	Valida la respuesta de login. 0 es datos incorrectos,
	1 es usuario logueado y 2 es usuario bloqueado
*/
int32_t verificar_log(int32_t log){
	if(log == 0){
		return 0;
	}
	else if(log == 1){
		return 1;
	}
	else{
		return 2;
	}
}

/*
	solicita los usuarios de la base de datos, si están bloqueados y 
	ultima conexión
*/
void names_request(){

	char* encabezado = "[Usuario] - [Bloqueado] - [Ultima conexion]\n";
	size_t size = strlen(encabezado);
	char* guion = " - ";
	char* salto = "\n";
	char aux[3] = "";	//si es bloqueado
	for(int32_t i = 0; i < CANTIDAD_USUARIOS; i++) {
		size = size + strlen(usuarios[i]->usuario) + strlen(guion) + 
				strlen(aux) + strlen(guion) + 
				strlen(usuarios[i]->ultima_conexion);
		if(i < CANTIDAD_USUARIOS - 1){
			size = size + strlen(salto);
		}
	}
	char credenciales[size];
	if(credenciales == NULL){
		printf("%sError alocando memoria%s\n",KRED,KNRM);
		exit(1);
	}
	memset(credenciales,0,sizeof(credenciales));
	sprintf(credenciales,"%s",encabezado);

	for(int32_t i=0; i<CANTIDAD_USUARIOS; i++){
		if( atoi(usuarios[i]->intentos) < 3){
			sprintf(aux,"No");
		}
		else{
			sprintf(aux,"Si");
		}

		char aux2[strlen(credenciales)]; //variabe donde guardo toda la información
		sprintf(aux2,"%s",credenciales);
		sprintf(credenciales,"%s%s%s%s%s%s%s",aux2,usuarios[i]->usuario,	// en cada itereacción guarda los datos
			guion,aux,guion,usuarios[i]->ultima_conexion,salto);
	}

	send_to_queue((long)NAMES_RESPONSE, credenciales);
}

/*
	toma los datos y los emplea para cambiar la contraseña
	luego envía al server la respuesta
*/
void password_change(){

	char credenciales[strlen(mensaje)];
	if(credenciales == NULL){
		printf("%sError alocando memoria%s\n",KRED,KNRM);
		exit(1);
	}
	sprintf(credenciales,"%s",mensaje);

	change_password(credenciales);

	send_to_queue((long)PASSWORD_CHANGE_RESPONSE,"PASSWORD_CHANGE_RESPONSE");
}

/*
	Levanta la contraseña que envía el server para cambiar
	busca el usuario que matchea con el que está logueado
	y le guarda la nueva contraseña en el campo correspondiente
*/
void change_password(char* datos){

	char* aux = strtok(datos,"\n");
	char new_password[strlen(aux)];
	sprintf(new_password,"%s",aux);
	
	for(int32_t i=0; i<CANTIDAD_USUARIOS; i++){
		if(strcmp(user,usuarios[i]->usuario) == 0){
			sprintf(usuarios[i]->password,"%s",new_password);
		}
	}

	actualizar_bd();

}

/*
	actualiza la base de datos, escribiendo en el archivo
	lo que haya en el arreglo buffer
*/
void actualizar_bd(){
	FILE *file;
	file = fopen(base_datos_usuarios, "w+"); //para leer y escribir

	if ( file != NULL ) {
	//se empleó fputs y no resultó ser útil
		for(int32_t i=0; i<CANTIDAD_USUARIOS; i++){
			fprintf(file,"%s",usuarios[i]->usuario);
			fprintf(file,"%s","\n");
			fprintf(file,"%s",usuarios[i]->password);
			fprintf(file,"%s","\n");
			fprintf(file,"%s",usuarios[i]->intentos);
			fprintf(file,"%s","\n");
			fprintf(file,"%s",usuarios[i]->ultima_conexion);
			fprintf(file,"%s","\n");
		}
		
	}
	fclose(file);

}
