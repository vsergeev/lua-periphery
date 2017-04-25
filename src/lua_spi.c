/*
 * lua-periphery by vsergeev
 * https://github.com/vsergeev/lua-periphery
 * License: MIT
 */

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <c-periphery/src/spi.h>
#include "lua_periphery.h"
#include "lua_compat.h"

/*
local periphery = require('periphery')
local SPI = periphery.SPI

-- Module Version
SPI.version         <string>

-- Constructor
spi = SPI(device <path string>, mode <number>, max_speed <number>)
spi = SPI{device=<path string>, mode=<number>, max_speed=<number>, bit_order="msb", bits_per_word=8, extra_flags=0}

-- Methods
spi:transfer(data <table>) --> <table>
spi:close()

-- Properties
spi.fd              immutable <number>
spi.mode            mutable <number>
spi.max_speed       mutable <number>
spi.bit_order       mutable <number>
spi.bits_per_word   mutable <number>
spi.extra_flags     mutable <number>
*/

/* Define a new error for malloc() required in read/write */
#define SPI_ERROR_ALLOC    (SPI_ERROR_CLOSE-1)

static const char *spi_error_code_strings[] = {
    [-SPI_ERROR_ARG]        = "SPI_ERROR_ARG",
    [-SPI_ERROR_OPEN]       = "SPI_ERROR_OPEN",
    [-SPI_ERROR_QUERY]      = "SPI_ERROR_QUERY",
    [-SPI_ERROR_CONFIGURE]  = "SPI_ERROR_CONFIGURE",
    [-SPI_ERROR_TRANSFER]   = "SPI_ERROR_TRANSFER",
    [-SPI_ERROR_CLOSE]      = "SPI_ERROR_CLOSE",
    [-SPI_ERROR_ALLOC]      = "SPI_ERROR_ALLOC",
};

static int lua_spi_error(lua_State *L, enum spi_error_code code, int c_errno, const char *fmt, ...) {
    char message[128];
    va_list ap;

    va_start(ap, fmt);

    /* Create error table */
    lua_newtable(L);
    /* .code string */
    lua_pushstring(L, spi_error_code_strings[-code]);
    lua_setfield(L, -2, "code");
    /* .c_errno number */
    lua_pushinteger(L, c_errno);
    lua_setfield(L, -2, "c_errno");
    /* .message string */
    vsnprintf(message, sizeof(message), fmt, ap);
    lua_pushstring(L, message);
    lua_setfield(L, -2, "message");

    va_end(ap);

    /* Set error metatable on it */
    luaL_getmetatable(L, "periphery.error");
    lua_setmetatable(L, -2);

    return lua_error(L);
}

static void lua_spi_checktype(lua_State *L, int index, int type) {
    if (lua_type(L, index) != type)
        lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: invalid argument #%d (%s expected, got %s)", index, lua_typename(L, type), lua_typename(L, lua_type(L, index)));
}

static int lua_spi_open(lua_State *L) {
    spi_t *spi;
    const char *device;
    unsigned int mode;
    uint32_t max_speed;
    spi_bit_order_t bit_order;
    uint8_t bits_per_word;
    uint8_t extra_flags;
    int ret;

    spi = luaL_checkudata(L, 1, "periphery.SPI");

    /* Initialize file descriptor to an invalid value, in case an error occurs
     * below and gc later close()'s this object. */
    spi->fd = -1;

    /* Default settings of optional arguments */
    bit_order = MSB_FIRST;
    bits_per_word = 8;
    extra_flags = 0;

    /* Arguments passed in table form */
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "device");
        if (!lua_isstring(L, -1))
            return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: invalid type on table argument 'device', should be string");
        lua_getfield(L, 2, "mode");
        if (!lua_isnumber(L, -1))
            return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: invalid type on table argument 'mode', should be number");
        lua_getfield(L, 2, "max_speed");
        if (!lua_isnumber(L, -1))
            return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: invalid type on table argument 'max_speed', should be number");

        device = lua_tostring(L, -3);
        mode = lua_tounsigned(L, -2);
        max_speed = lua_tounsigned(L, -1);

        /* Optional bit_order */
        lua_getfield(L, 2, "bit_order");
        if (lua_isstring(L, -1)) {
            const char *s;
            s = lua_tostring(L, -1);
            if (strcmp(s, "msb") == 0)
                bit_order = MSB_FIRST;
            else if (strcmp(s, "lsb") == 0)
                bit_order = LSB_FIRST;
            else
                return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: invalid table argument 'bit_order', should be 'msb' or 'lsb'");
        } else if (!lua_isnil(L, -1)) {
            return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: invalid type on table argument 'bit_order', should be string");
        }

        /* Optional bits_per_word */
        lua_getfield(L, 2, "bits_per_word");
        if (lua_isnumber(L, -1))
            bits_per_word = lua_tounsigned(L, -1);
        else if (!lua_isnil(L, -1))
            return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: invalid type on table argument 'bits_per_word', should be number");

        /* Optional extra_flags */
        lua_getfield(L, 2, "extra_flags");
        if (lua_isnumber(L, -1))
            extra_flags = lua_tounsigned(L, -1);
        else if (!lua_isnil(L, -1))
            return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: invalid type on table argument 'extra_flags', should be number");

    /* Arguments passed normally */
    } else {
        lua_spi_checktype(L, 2, LUA_TSTRING);
        lua_spi_checktype(L, 3, LUA_TNUMBER);
        lua_spi_checktype(L, 4, LUA_TNUMBER);

        device = lua_tostring(L, 2);
        mode = lua_tounsigned(L, 3);
        max_speed = lua_tounsigned(L, 4);
    }

    if ((ret = spi_open_advanced(spi, device, mode, max_speed, bit_order, bits_per_word, extra_flags)) < 0)
        return lua_spi_error(L, ret, spi_errno(spi), spi_errmsg(spi));

    return 0;
}

static int lua_spi_new(lua_State *L) {
    /* Remove self table object */
    lua_remove(L, 1);

    /* Create handle userdata */
    lua_newuserdata(L, sizeof(spi_t));
    /* Set SPI metatable on it */
    luaL_getmetatable(L, "periphery.SPI");
    lua_setmetatable(L, -2);
    /* Move userdata to the beginning of the stack */
    lua_insert(L, 1);

    /* Call open */
    lua_spi_open(L);

    /* Leave only userdata on the stack */
    lua_settop(L, 1);

    return 1;
}

static int lua_spi_transfer(lua_State *L) {
    spi_t *spi;
    uint8_t *buf;
    unsigned int i, len;
    int ret;

    spi = luaL_checkudata(L, 1, "periphery.SPI");
    lua_spi_checktype(L, 2, LUA_TTABLE);

    len = luaL_len(L, 2);

    if ((buf = malloc(len)) == NULL)
        return lua_spi_error(L, SPI_ERROR_ALLOC, errno, "Error: allocating memory");

    /* Convert byte table to byte buffer */
    for (i = 0; i < len; i++) {
        lua_pushunsigned(L, i+1);
        lua_gettable(L, -2);
        if (!lua_isnumber(L, -1)) {
            free(buf);
            return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: invalid element index %d in bytes table.", i+1);
        }

        buf[i] = lua_tounsigned(L, -1);
        lua_pop(L, 1);
    }

    if ((ret = spi_transfer(spi, buf, buf, len)) < 0) {
        free(buf);
        return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));
    }

    /* Convert byte buffer back to bytes table */
    for (i = 0; i < len; i++) {
        lua_pushunsigned(L, i+1);
        lua_pushunsigned(L, buf[i]);
        lua_settable(L, -3);
    }

    free(buf);

    return 1;
}

static int lua_spi_close(lua_State *L) {
    spi_t *spi;
    int ret;

    spi = luaL_checkudata(L, 1, "periphery.SPI");

    if ((ret = spi_close(spi)) < 0)
        return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));

    return 0;
}

static int lua_spi_tostring(lua_State *L) {
    spi_t *spi;
    char spi_str[128];

    spi = luaL_checkudata(L, 1, "periphery.SPI");

    spi_tostring(spi, spi_str, sizeof(spi_str));

    lua_pushstring(L, spi_str);

    return 1;
}

static int lua_spi_index(lua_State *L) {
    spi_t *spi;
    const char *field;

    if (!lua_isstring(L, 2))
        return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: unknown method or property");

    field = lua_tostring(L, 2);

    /* Look up method in metatable */
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, field);
    if (!lua_isnil(L, -1))
        return 1;

    spi = luaL_checkudata(L, 1, "periphery.SPI");

    if (strcmp(field, "fd") == 0) {
        lua_pushinteger(L, spi_fd(spi));
        return 1;
    } else if (strcmp(field, "mode") == 0) {
        unsigned int mode;
        int ret;

        if ((ret = spi_get_mode(spi, &mode)) < 0)
            return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));

        lua_pushunsigned(L, mode);
        return 1;
    } else if (strcmp(field, "max_speed") == 0) {
        uint32_t speed;
        int ret;

        if ((ret = spi_get_max_speed(spi, &speed)) < 0)
            return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));

        lua_pushunsigned(L, speed);
        return 1;
    } else if (strcmp(field, "bit_order") == 0) {
        spi_bit_order_t bit_order;
        int ret;

        if ((ret = spi_get_bit_order(spi, &bit_order)) < 0)
            return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));

        switch (bit_order) {
            case MSB_FIRST: lua_pushstring(L, "msb"); break;
            case LSB_FIRST: lua_pushstring(L, "lsb"); break;
            default: lua_pushstring(L, "unknown"); break;
        }
        return 1;
    } else if (strcmp(field, "bits_per_word") == 0) {
        uint8_t bits_per_word;
        int ret;

        if ((ret = spi_get_bits_per_word(spi, &bits_per_word)) < 0)
            return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));

        lua_pushunsigned(L, bits_per_word);
        return 1;
    } else if (strcmp(field, "extra_flags") == 0) {
        uint8_t extra_flags;
        int ret;

        if ((ret = spi_get_extra_flags(spi, &extra_flags)) < 0)
            return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));

        lua_pushunsigned(L, extra_flags);
        return 1;
    }

    return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: unknown property");
}

static int lua_spi_newindex(lua_State *L) {
    spi_t *spi;
    const char *field;

    spi = luaL_checkudata(L, 1, "periphery.SPI");

    if (!lua_isstring(L, 2))
        return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: unknown property");

    field = lua_tostring(L, 2);

    if (strcmp(field, "fd") == 0)
        return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: immutable property");
    else if (strcmp(field, "mode") == 0) {
        unsigned int mode;
        int ret;

        lua_spi_checktype(L, 3, LUA_TNUMBER);
        mode = lua_tounsigned(L, 3);

        if ((ret = spi_set_mode(spi, mode)) < 0)
            return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));

        return 0;
    } else if (strcmp(field, "max_speed") == 0) {
        uint32_t max_speed;
        int ret;

        lua_spi_checktype(L, 3, LUA_TNUMBER);
        max_speed = lua_tounsigned(L, 3);

        if ((ret = spi_set_max_speed(spi, max_speed)) < 0)
            return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));

        return 0;
    } else if (strcmp(field, "bit_order") == 0) {
        const char *s;
        spi_bit_order_t bit_order;
        int ret;

        lua_spi_checktype(L, 3, LUA_TSTRING);

        s = lua_tostring(L, 3);
        if (strcmp(s, "msb") == 0)
            bit_order = MSB_FIRST;
        else if (strcmp(s, "lsb") == 0)
            bit_order = LSB_FIRST;
        else
            return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: invalid bit_order, should be 'msb' or 'lsb'");

        if ((ret = spi_set_bit_order(spi, bit_order)) < 0)
            return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));

        return 0;
    } else if (strcmp(field, "bits_per_word") == 0) {
        uint8_t bits_per_word;
        int ret;

        lua_spi_checktype(L, 3, LUA_TNUMBER);
        bits_per_word = lua_tounsigned(L, 3);

        if ((ret = spi_set_bits_per_word(spi, bits_per_word)) < 0)
            return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));

        return 0;
    } else if (strcmp(field, "extra_flags") == 0) {
        uint8_t extra_flags;
        int ret;

        lua_spi_checktype(L, 3, LUA_TNUMBER);
        extra_flags = lua_tounsigned(L, 3);

        if ((ret = spi_set_extra_flags(spi, extra_flags)) < 0)
            return lua_spi_error(L, ret, spi_errno(spi), "Error: %s", spi_errmsg(spi));

        return 0;
    }

    return lua_spi_error(L, SPI_ERROR_ARG, 0, "Error: unknown property");
}

static const struct luaL_Reg periphery_spi_m[] = {
    {"close", lua_spi_close},
    {"transfer", lua_spi_transfer},
    {"__gc", lua_spi_close},
    {"__tostring", lua_spi_tostring},
    {"__index", lua_spi_index},
    {"__newindex", lua_spi_newindex},
    {NULL, NULL}
};

LUALIB_API int luaopen_periphery_spi(lua_State *L) {
    /* Create periphery.SPI metatable */
    luaL_newmetatable(L, "periphery.SPI");
    /* Set metatable functions */
    const struct luaL_Reg *funcs = (const struct luaL_Reg *)periphery_spi_m;
    for (; funcs->name != NULL; funcs++) {
        lua_pushcclosure(L, funcs->func, 0);
        lua_setfield(L, -2, funcs->name);
    }
    /* Set metatable properties */
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");

    /* Create {__call = lua_spi_new, __metatable = "protected metatable", version = ...} table */
    lua_newtable(L);
    lua_pushcclosure(L, lua_spi_new, 0);
    lua_setfield(L, -2, "__call");
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");
    /* Set it as the metatable for the periphery.SPI metatable */
    lua_setmetatable(L, -2);

    lua_pushstring(L, LUA_PERIPHERY_VERSION);
    lua_setfield(L, -2, "version");

    return 1;
}

