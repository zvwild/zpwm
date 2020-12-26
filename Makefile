zpwm: zpwm.c
	gcc zpwm.c -O3 -s -o zpwm -Wall -Wextra -pedantic -std=c11 -lzip

install: zpwm
	cp ./zpwm /usr/bin/zpwm
