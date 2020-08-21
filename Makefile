FLAGS = -g -Wall -Werror -Wextra -Wconversion -pedantic

all:
	cppcheck ./
	gcc $(FLAGS) src/cliente.c src/funciones.c -o bin/client
	gcc $(FLAGS) src/servidor.c src/funciones.c -o bin/server
	gcc $(FLAGS) src/auth.c src/funciones.c -o bin/auth
	gcc $(FLAGS) src/file.c src/funciones.c -o bin/file

clean:
	rm bin/*
	rm -r html/*
	rm latex/*

docs:
	doxygen Doxyfile