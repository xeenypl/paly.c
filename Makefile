all: play
	
play: play.c
	gcc -O2 -o play play.c -lmpg123 -lao
