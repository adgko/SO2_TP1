#include "../include/recursos.h"


void configurar_socket();
void ejecutar_bin();
void escuchando();
void conectar_cliente();
void rec_user();
void enviar_a_cola_local(long , char [MENSAJE_TAM]);
void enviar_a_cliente(char*);
void verificar_respuesta();
void middle();
void validar_comando();
void exit_command();
void user_command( char*, char*);
void user_ls();
void user_passwd(char*);
void file_command(char*, char*);
void file_ls();
void file_down(char*);
void envio_mensaje_aux();
void unknown_command();
