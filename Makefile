sim: src/common.c src/cache.c src/main.c
	gcc -o sim src/main.c src/cache.c src/common.c -I./include -Wall -Werror -lm

clean: 
	rm -f sim
