FLAGS = -std=gnu11 -Wall -Werror -Wextra -Wconversion -pedantic -lssl -lcrypto -g

all: check cliente servidor

cliente:
	mkdir -p ./bin
	gcc src/cliente.c src/funciones.c -o bin/client $(FLAGS)

servidor:
	gcc src/servidor.c src/funciones.c -o bin/server $(FLAGS)
	gcc src/auth.c src/funciones.c -o bin/auth $(FLAGS)
	gcc src/file.c src/funciones.c -o bin/file $(FLAGS)

clean:
	rm bin/*
	rm -r html/*
	rm latex/*

docs:
	doxygen Doxyfile

check:
	cppcheck ./
