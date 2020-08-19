FLAGS = -g -Wall -Werror -Wextra -Wconversion -pedantic

all:
	gcc $(FLAGS) src/cliente.c src/funciones.c -o bin/client
	gcc $(FLAGS) src/servidor.c src/funciones.c -o bin/server
	gcc $(FLAGS) src/auth.c src/funciones.c -o bin/auth
	gcc $(FLAGS) src/file.c src/funciones.c -o bin/file

clean:
	rm bin/client
	rm bin/server
	rm bin/auth
	rm bin/file