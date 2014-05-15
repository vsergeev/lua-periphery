LIB = periphery.so
SOURCES = src/lua_periphery.c src/lua_mmio.c src/lua_gpio.c src/lua_spi.c src/lua_i2c.c src/lua_serial.c

C_PERIPHERY = c-periphery
C_PERIPHERY_LIB = $(C_PERIPHERY)/periphery.a

ifndef LUA
LUA = lua
endif

ifndef LUA_INCDIR
LUA_CFLAGS = $(shell pkg-config --cflags $(LUA))
else
LUA_CFLAGS = -I$(LUA_INCDIR)
endif

###########################################################################

CFLAGS += -Wall -Wextra -Wno-unused-parameter $(DEBUG) -fPIC $(LUA_CFLAGS) -I.
LDFLAGS += -shared

###########################################################################

all: $(LIB)

$(LIB): $(C_PERIPHERY_LIB) $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) $(C_PERIPHERY_LIB) -o $@ $(LDFLAGS)

$(C_PERIPHERY_LIB):
	cd $(C_PERIPHERY); make

clean:
	cd $(C_PERIPHERY); make clean;
	rm -rf $(LIB)

