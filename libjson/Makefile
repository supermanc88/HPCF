CC = gcc
CFLAGS = -O0 -g -Wall
TARGET = libparson.a
OBJS = parson.o

all: $(TARGET)
	$(CC) $(CFLAGS) -o test test.c $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $(TARGET) $(OBJS)
	# $(RANLIB) $(TARGET)



.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS) test