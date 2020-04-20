all: play
	
play: main.c
	gcc -O2 -o play main.c -lmpg123 -lao
