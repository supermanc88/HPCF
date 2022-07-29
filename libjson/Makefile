CC = gcc
CFLAGS = -O0 -g -Wall
TARGET = libparson.a
OBJS = parson.o

all: $(TARGET)
	# build test program
	$(CC) $(CFLAGS) -o test test.c $(TARGET)

$(TARGET): $(OBJS)
	# build library
	$(AR) rcs $(TARGET) $(OBJS)
	# $(RANLIB) $(TARGET)



.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS) test