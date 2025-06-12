CFLAGS=-Wall -Wextra -pedantic -ggdb
FILES=ast lexer parser scope

debug:
	for f in $(FILES) ; do \
		gcc -c src/$$f.c -o build/$$f.o ; \
	done

	gcc $(CFLAGS) -o ./bin/saad \
		build/ast.o \
		build/lexer.o \
		build/parser.o \
		build/scope.o \
		src/main.c

release:
	gcc src/main.c -o bin/saad -Wall -Wextra -pedantic -O3