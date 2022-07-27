TARGET = liblogin_auth.so
CC = gcc
CFLAGS = -g -fPIC -shared
INCLUDE = -I.
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
LIBS = ../../libjson/libparson.a

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $(OBJS) $(LIBS) -o $(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)