all: wish
.PHONY: all

wish: wish.c
	gcc -g wish.c -o wish

clean:
	rm -f wish
	rm -f *.output.txt
.PHONY: clean

remake: clean all
.PHONY: remake