SOURCES = udp_txp.c raw_tcp.c udp.c udp_txp_server.c udp_txp_client.c
OBJECTS = $(SOURCES:%.c=%.o)
CFLAGS  = -Wall -pedantic -Wextra -Werror

all: $(OBJECTS)
	gcc $? -o udp_txp

