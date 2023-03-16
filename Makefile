all: wish

wish:
	gcc wish.c -o wish

clean:
	rm -f wish