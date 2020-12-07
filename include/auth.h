#include "../include/recursos.h"

#define base_datos_usuarios "../archivos/users/users_credentials"

/*
	Funciones empleadas por auth
*/
void leer_bd();
void listen_user();
int32_t login(char*);
int32_t get_bloqueado();
void set_ultima_conexion();
int32_t set_intentos(int32_t);
void login_request();
void names_request();
void password_change();
void change_password(char*);
void actualizar_bd();

/*
	Guarda el usuario y contraseña para la autenticación y búsqueda
	También guarda los intentos que este realizaó y si está bloqueado o no
	Por último, guarda la última conexión de este
*/
typedef struct {
  char usuario[USUARIO_TAM];
  char password[CLAVE_TAM];
  char intentos[INTENTOS_TAM];
  char ultima_conexion[LAST_CONECTION_SIZE];
} Usuario;

/*
	Variables empleadas por el auth
*/
Usuario* usuarios[CANTIDAD_USUARIOS];
char* mensaje;
char user[USUARIO_TAM];		//el usuario ya logueado
char user_aux[USUARIO_TAM];	//el usuario siendo validado