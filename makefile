xkp: xkp.c audio.c
	$(CC) -O3 --std=c99 -pedantic -Wall xkp.c audio.c -o xkp -lX11 -lXi -lportaudio -lm
clean:
	rm --force xkp
