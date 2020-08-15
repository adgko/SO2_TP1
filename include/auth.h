#include "../include/recursos.h"

#define base_datos_usuarios "../archivos/users/users_credentials"

void leer_bd();
void listen_user();
int32_t login(char*);
void login_request();
char verificar_log(int32_t*);
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