all: wish
.PHONY: all

wish:
	gcc -g wish.c -o wish

clean:
	rm -f wish
.PHONY: clean

remake: clean all
.PHONY: remake