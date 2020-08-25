#include "../include/recursos.h"

#define IMAGES_PATH "../archivos/cosa/"

void configurar_socket();
void Lista_de_archivos();
void archivos_error(int32_t);
void guardar_datos(int32_t,struct dirent*,char*);
void calc_size(int32_t,char*);
void escuchando();
void listen_user();
void files_request();
void download_request();
void conectar_cliente();
void conectar_enviar(int32_t );
void enviar_a_cliente(char* );
void enviar_archivo(int32_t );
void verificar_path(char* );
void enviar_a_cliente_archivo(char* path);

/*
	La estructura que guardará la información de las imágenes
*/
typedef struct {
  int32_t index;							//el indice sirve para buscar y ordenar los archivos
  char nombre[ARCHIVO_NAME_SIZE];			//nombre del archivo
  char formato[ARCHIVO_FORMAT_SIZE];		//formato del archivo
  float size;								//tamaño del archivo
  char hash[MD5_DIGEST_LENGTH];				//hash del archivo a ser comparado en el grabado
} Archivo;