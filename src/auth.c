#include "../include/auth.h"

Usuario* usuarios[CANTIDAD_USUARIOS];
char* mensaje;
char* user;

int32_t main(){

	printf("%sIniciando Auth Service%s\n", KBLU,KNRM );
	leer_bd();

	while(1){		//se queda esperando que en la cola haya algo para él
		listen_user();
		sleep(TIME_SLEEP);
	}
	exit(0);
}

/*
	Abre un archivo txt, guarda todas las líneas y las almacena como
	campos de la estructura Usuario
*/
void leer_bd() {

	char lineas[CANTIDAD_USUARIOS * USUARIO_CAMPOS][RENGLON];
	
	FILE *file;
	file = fopen(base_datos_usuarios, "r"); // para leer

	if ( file != NULL ) {
      	char line[RENGLON];
		int32_t index = 0;
      	while ( fgets( line, RENGLON, file ) != NULL ) {
				sprintf(lineas[index], "%s", line);
				index++;
      	}

      	fclose(file);
 	}
	else{
		printf("%sError al Leer la Base de Datos%s\n", KRED,KNRM);
		exit(1);
	}
	/*
		Por cada usuario y por cada campo de todo el grupo, se guarda 
		los datos obtenidos del documento
	*/
	for(int32_t i = 0,k = 0; i < CANTIDAD_USUARIOS && k < CANTIDAD_USUARIOS*USUARIO_CAMPOS;i++) {
		int32_t usuario_index = i;
		usuarios[usuario_index] = malloc(sizeof(Usuario));
		if(usuarios[usuario_index] == NULL) {
			printf("%sError alocando memoria en usuarios%s\n",KRED,KNRM);
			exit(1);
		}

		sprintf(usuarios[usuario_index]->usuario, "%s", lineas[k++]);
		usuarios[usuario_index]->usuario[strlen(usuarios[usuario_index]->usuario) - 1] = '\0';
		sprintf(usuarios[usuario_index]->password, "%s", lineas[k++]);
		usuarios[usuario_index]->password[strlen(usuarios[usuario_index]->password) - 1] = '\0';
		sprintf(usuarios[usuario_index]->intentos, "%s", lineas[k++]);
		usuarios[usuario_index]->intentos[strlen(usuarios[usuario_index]->intentos) - 1] = '\0';
		sprintf(usuarios[usuario_index]->ultima_conexion, "%s", lineas[k++]);
		usuarios[usuario_index]->ultima_conexion[strlen(usuarios[usuario_index]->ultima_conexion) - 1] = '\0';
	}
	//printf("%sHola\n",KRED );
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
		printf("viendo si anda6\n");
		password_change();
	}
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

		int32_t log = login(credenciales);

		int32_t rta = verificar_log(log);

		char aux[3] = "";
		sprintf(aux,"%d",rta);
		printf("%srevisando login_request%s\n",KCYN,KNRM);

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

	printf("Verificando que onda el log\n");
	for(int32_t i = 0; i < CANTIDAD_USUARIOS; i++) {
		if( strcmp(usuario, usuarios[i]->usuario) == 0 ) {
			printf("%s%s\n", usuarios[i]->usuario," es este");
			int32_t aux1 = atoi(usuarios[i]->intentos);
			if( aux1 < 3) {
				if( strcmp(password, usuarios[i]->password) == 0 ) {
					//set_ultima_conexion(usuarios[i]->usuario);
					printf("entro aca?\n");
					sprintf(usuarios[i]->intentos,"%s", "0");		//al loguear, setea los intentos en 0
					sprintf(user,"%s",usuarios[i]->usuario);
					actualizar_bd();
					printf("va salir?\n");
					return 1;
				}
				else{
					int32_t aux2 = 0;
					printf("fallo el password\n");
					aux2 = atoi(usuarios[i]->intentos);
					aux2++;
					printf("intentos: %d\n", aux2);
					sprintf(usuarios[i]->intentos,"%d", aux2);   //como es incorrecto, aumenta los intentos en 1
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

	printf("%s3%s",KMAG,KNRM);

	char credenciales[strlen(mensaje)];
	if(credenciales == NULL){
		printf("%sError alocando memoria%s\n",KRED,KNRM);
		exit(1);
	}
	sprintf(credenciales,"%s",mensaje);

	change_password(credenciales);

	actualizar_bd();

	send_to_queue((long)PASSWORD_CHANGE_RESPONSE,"PASSWORD_CHANGE_RESPONSE");
}


void change_password(char* datos){

	char* aux = strtok(datos,"\n");
	//char usuario[strlen(aux)];
	//sprintf(usuario,"%s",aux);
	//aux = strtok(NULL,"\0");
	char new_password[strlen(aux)];
	sprintf(new_password,"%s",aux);
	
	for(int32_t i=0; i<CANTIDAD_USUARIOS; i++){
		if(strcmp(user,usuarios[i]->usuario) == 0){
			sprintf(usuarios[i]->password,"%s",new_password);
		}
	}

}

/*
	actualiza la base de datos, escribiendo en el archivo
	lo que haya en el arreglo buffer
*/
void actualizar_bd(){
	FILE *file;
	file = fopen(base_datos_usuarios, "w+"); //para leer y escribir

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
		fclose(file);
	

}