#include "../include/recursos.h"

#define IMAGES_PATH "../archivos/img/"

/*
	Funciones empleadas por file
*/
void configurar_socket();
void Lista_de_archivos();
void archivos_error(int32_t);
void guardar_datos(int32_t,struct dirent*,char*);
void calc_size(int32_t,char*);
void escuchando();
void listen_user();
void files_request();
void download_request();
void send_image(char*, long*);
void start_listening(int , char*);
void conectar_cliente();
void conectar_enviar(int32_t );
void enviar_a_cliente(char* );
void enviar_archivo(int32_t);
void verificar_path(char* );
void enviar_a_cliente_archivo(char* path);
void confirmacion_cliente();

/*
	La estructura que guardará la información de las imágenes
*/
typedef struct {
  int32_t index;							//el indice sirve para buscar y ordenar los archivos
  char nombre[ARCHIVO_NAME_SIZE];			//nombre del archivo
  char formato[ARCHIVO_FORMAT_SIZE];		//formato del archivo
  float size;								//tamaño del archivo
  char hash[MD5_DIGEST_LENGTH*2+1];				//hash del archivo a ser comparado en el grabado
} Archivo;

/*
	Variables empleadas por el file
*/
int32_t sockfd, sock_cli, puerto;
ssize_t n;							// hubo que declarar n como ssize_t para que no pierda información al usarse en send() y recv()
struct sockaddr_in serv_addr;
struct sockaddr_in client_addr;
char buffer[TAM],direccion[20];
uint32_t client_len;				//tamaño de la dirección del cliente
char* mensaje_resp;
Archivo* archivos[CANT_ARCHIVOS]; 
int32_t confirm_flag = 0;
char nombre_archivo[TAM];