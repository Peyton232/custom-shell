CC=gcc
CFLAGS=-Wall -g
shell: major2.c
	$(CC) -o shell $(CFLAGS) major2.c
clean:
	$(RM) shell
run:
	./shell