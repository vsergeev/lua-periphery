LIB = periphery.so
C_PERIPHERY = c-periphery
C_PERIPHERY_LIB = $(C_PERIPHERY)/periphery.a
SOURCES = src/lua_periphery.c src/lua_mmio.c src/lua_gpio.c src/lua_spi.c src/lua_i2c.c src/lua_serial.c

ifndef LUA
LUA = lua
endif

###########################################################################

CC = $(CROSS)gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter $(DEBUG)
CFLAGS += -fPIC
CFLAGS += $(shell pkg-config --cflags $(LUA)) -I/usr/include -I.
LDFLAGS = -shared

###########################################################################

all: $(LIB)

$(LIB): $(C_PERIPHERY_LIB) $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) $(C_PERIPHERY_LIB) -o $@ $(LDFLAGS)

$(C_PERIPHERY_LIB):
	cd $(C_PERIPHERY); make

clean:
	cd $(C_PERIPHERY); make clean
	rm -rf $(LIB)

