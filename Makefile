DRC_INC=$(shell pkg-config --cflags cray-drc)
DRC_LIB=$(shell pkg-config --libs cray-drc)

MARGO_INC=$(shell pkg-config --cflags margo)
MARGO_LIB=$(shell pkg-config --libs margo)

all: server client

server: server_proxy.o
	cc -o server server_proxy.o $(MARGO_LIB) $(DRC_LIB)

client: client_proxy.o
	cc -o client client_proxy.o $(MARGO_LIB) $(DRC_LIB)

server_proxy.o: server_proxy.c
	cc -Wall -g -c server_proxy.c $(MARGO_INC) $(DRC_INC)


client_proxy.o: client_proxy.c
	cc -Wall -g -c client_proxy.c $(MARGO_INC) $(DRC_INC)

clean:
	rm -f server client *.o
