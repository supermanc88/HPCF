objects = $(patsubst %.c,%.o,$(wildcard *.c))
target = crypto_cloud
CFLAGS = -g
LIBS = -lpthread -ldl

$(target): $(objects)
	$(CC) $(CFLAGS) -o $@ $(objects) $(LIBS)

.PHONY: clean
clean:
	rm -f $(target)
	rm -f $(objects)