objects = $(patsubst %.c,%.o,$(wildcard *.c))
CFLAGS = -g
LIBS = -lpthread -ldl
DEFINES = -HPCF_DEBUG

all: libjson modules crypto_cloud

libjson: none
	# build libjson
	$(MAKE) -C libjson

modules: none
	# build modules
	$(MAKE) -C modules/login_authentication

crypto_cloud: $(objects)
	$(CC) $(CFLAGS) -o $@ $(objects) $(LIBS)

none:
	# build

.PHONY: clean
clean:
	rm -f crypto_cloud
	rm -f $(objects)
	rm -f plugins/*.so

	# clean libjson
	$(MAKE) -C libjson clean

	# clean modules
	$(MAKE) -C modules/login_authentication clean