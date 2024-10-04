debug:
	gcc src/main.c -o bin/saad -Wall -Wextra -pedantic -ggdb

release:
	gcc src/main.c -o bin/saad -Wall -Wextra -pedantic -O3