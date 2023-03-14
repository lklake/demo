#
# Makefile
#

all: kbd_server

%.o: %.c
	gcc -Wall -c $< `pkg-config --cflags dbus-1` `pkg-config --libs x11`

kbd_server: kbd_server.o
	gcc kbd_server.o -o kbd_server `pkg-config --libs dbus-1` `pkg-config --libs x11`

.PHONY: clean
clean:
	rm *.o kbd_server
