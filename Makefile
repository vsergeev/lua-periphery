LIB = periphery.so
SRCS = src/lua_periphery.c src/lua_mmio.c src/lua_gpio.c src/lua_spi.c src/lua_i2c.c src/lua_serial.c

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

CFLAGS += -std=c99 -pedantic -D_XOPEN_SOURCE=700
CFLAGS += -Wall -Wextra -Wno-unused-parameter $(DEBUG) -fPIC -I. $(LUA_CFLAGS)
LDFLAGS += -shared

###########################################################################

.PHONY: all
all: $(LIB)

.PHONY: clean
clean:
	cd $(C_PERIPHERY) && $(MAKE) clean
	rm -rf $(LIB)

.PHONY: install
install:
	mkdir -p $(LUA_LIBDIR)
	cp $(LIB) $(LUA_LIBDIR)/$(LIB)

###########################################################################

$(LIB): $(C_PERIPHERY_LIB) $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) $(C_PERIPHERY_LIB) -o $@

$(C_PERIPHERY_LIB): $(C_PERIPHERY)/Makefile
	cd $(C_PERIPHERY); $(MAKE)

