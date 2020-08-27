# SO2_TP1

Programa Cliente-Servidor implementado en Lenguaje C, con servicio de autenticación y Descarga de archivos, para ser ejecutado por terminal Linux.
Emplea Socket, Signals, Message Queue, Funciones MD5 y Tabla de Particiones MBR

## Ejecución

./Server Dirección-IP Puerto
./Client Dirección-IP Puerto

Cliente solicita usuario y contraseña, y otorga tres intentos antes de bloquear el sistema.
Una vez logueado, a través de comandos se puede solicitar el listado de usuarios, estado y última conexión; cambio de contraseña; listado de archivos; y descarga de alguno como imagen booteable en un pendrive del lado del cliente.
