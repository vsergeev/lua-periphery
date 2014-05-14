LIB = periphery.so
PERIPHERY_LIB = periphery/periphery.a
SOURCES = src/lua_periphery.c src/lua_mmio.c src/lua_gpio.c src/lua_spi.c src/lua_i2c.c src/lua_serial.c

ifndef LUA
LUA = lua
endif

###########################################################################

CC = $(CROSS)gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter $(DEBUG)
CFLAGS += -fPIC
CFLAGS += $(shell pkg-config --cflags $(LUA)) -I/usr/include -I.
LDFLAGS = -shared #$(shell pkg-config --libs $(LUA))

###########################################################################

all: $(LIB)

$(LIB): $(PERIPHERY_LIB) $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) $(PERIPHERY_LIB) -o $@ $(LDFLAGS)

$(PERIPHERY_LIB):
	cd periphery; make

clean:
	cd periphery; make clean
	rm -rf $(LIB)

