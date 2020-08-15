#include "recursos.h"

void conf_puertos();
void conect_to_server();
void signal_handler();
void salida();
void enviar_a_socket(int32_t socket, char* mensaje);
int32_t login();
void comandos();
void enviar_comando();
void recibir_respuesta(int32_t socket);
void descargar();
void conect_to_files();
