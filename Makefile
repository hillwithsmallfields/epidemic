all: epidemic epidimages

epidemic: epidemic.c
	gcc -g -o epidemic -lm epidemic.c

epidimages: epidemic.c
	gcc -g -DPRODUCE_IMAGES=1 -o epidimages -lm -lpng epidemic.c
