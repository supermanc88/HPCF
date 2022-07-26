CC = gcc
CFLAGS = -O0 -g -Wall
TARGET = libparson.a
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

$(TARGET): $(OBJS)
	$(AR) rcs $(TARGET) $(OBJS)
	# $(RANLIB) $(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)