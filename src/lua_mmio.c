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

#include <c-periphery/src/mmio.h>
#include "lua_periphery.h"
#include "lua_compat.h"

/*
local periphery = require('periphery')
local MMIO = periphery.MMIO

-- Module Version
MMIO.version    <string>

-- Constructor
mmio = MMIO(address <number>, size <number>)
mmio = MMIO{address=<number>, size=<number>}

-- Methods
mmio:read32(offset <number>) --> <number>
mmio:read16(offset <number>) --> <number>
mmio:read8(offset <number>) --> <number>
mmio:read(offset <number>, length <number>) --> <table>
mmio:write32(offset <number>, value <number>)
mmio:write16(offset <number>, value <number>)
mmio:write8(offset <number>, value <number>)
mmio:write(offset <number>, data <table>)
mmio:close()

-- Properties
mmio.base       immutable <number>
mmio.size       immutable <number>
*/

/* Define a new error for malloc() required in read/write */
#define MMIO_ERROR_ALLOC    (MMIO_ERROR_UNMAP-1)

static const char *mmio_error_code_strings[] = {
    [-MMIO_ERROR_ARG]   = "MMIO_ERROR_ARG",
    [-MMIO_ERROR_OPEN]  = "MMIO_ERROR_OPEN",
    [-MMIO_ERROR_MAP]   = "MMIO_ERROR_MAP",
    [-MMIO_ERROR_CLOSE] = "MMIO_ERROR_CLOSE",
    [-MMIO_ERROR_UNMAP] = "MMIO_ERROR_UNMAP",
    [-MMIO_ERROR_ALLOC] = "MMIO_ERROR_ALLOC",
};

static int lua_mmio_error(lua_State *L, enum mmio_error_code code, int c_errno, const char *fmt, ...) {
    char message[128];
    va_list ap;

    va_start(ap, fmt);

    /* Create error table */
    lua_newtable(L);
    /* .code string */
    lua_pushstring(L, mmio_error_code_strings[-code]);
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

static void lua_mmio_checktype(lua_State *L, int index, int type) {
    if (lua_type(L, index) != type)
        lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: invalid argument #%d (%s expected, got %s)", index, lua_typename(L, type), lua_typename(L, lua_type(L, index)));
}

static int lua_mmio_open(lua_State *L) {
    mmio_t *mmio;
    off_t base;
    size_t size;
    int ret;

    mmio = luaL_checkudata(L, 1, "periphery.MMIO");

    /* Initialize MMIO pointer to an invalid value, in case an error occurs
     * below and gc later close()'s this object. */
    mmio->ptr = NULL;

    /* Arguments passed in table form */
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "base");
        if (!lua_isnumber(L, -1))
            return lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: invalid type of table argument 'base', should be number");
        lua_getfield(L, 2, "size");
        if (!lua_isnumber(L, -1))
            return lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: invalid type of table argument 'size', should be number");

        base = lua_tounsigned(L, -2);
        size = lua_tounsigned(L, -1);

    /* Arguments passed normally */
    } else {
        lua_mmio_checktype(L, 2, LUA_TNUMBER);
        lua_mmio_checktype(L, 3, LUA_TNUMBER);

        base = lua_tounsigned(L, 2);
        size = lua_tounsigned(L, 3);
    }

    if ((ret = mmio_open(mmio, base, size)) < 0)
        return lua_mmio_error(L, ret, mmio_errno(mmio), mmio_errmsg(mmio));

    return 0;
}

static int lua_mmio_new(lua_State *L) {
    /* Remove self table object */
    lua_remove(L, 1);

    /* Create handle userdata */
    lua_newuserdata(L, sizeof(mmio_t));
    /* Set MMIO metatable on it */
    luaL_getmetatable(L, "periphery.MMIO");
    lua_setmetatable(L, -2);
    /* Move userdata to the beginning of the stack */
    lua_insert(L, 1);

    /* Call open */
    lua_mmio_open(L);

    /* Leave only userdata on the stack */
    lua_settop(L, 1);

    return 1;
}

static int lua_mmio_read32(lua_State *L) {
    mmio_t *mmio;
    uint32_t value;
    uintptr_t offset;
    int ret;

    mmio = luaL_checkudata(L, 1, "periphery.MMIO");
    lua_mmio_checktype(L, 2, LUA_TNUMBER);

    offset = lua_tounsigned(L, 2);

    if ((ret = mmio_read32(mmio, offset, &value)) < 0)
        return lua_mmio_error(L, ret, mmio_errno(mmio), "Error: %s", mmio_errmsg(mmio));

    lua_pushunsigned(L, value);

    return 1;
}

static int lua_mmio_read16(lua_State *L) {
    mmio_t *mmio;
    uint16_t value;
    uintptr_t offset;
    int ret;

    mmio = luaL_checkudata(L, 1, "periphery.MMIO");
    lua_mmio_checktype(L, 2, LUA_TNUMBER);

    offset = lua_tounsigned(L, 2);

    if ((ret = mmio_read16(mmio, offset, &value)) < 0)
        return lua_mmio_error(L, ret, mmio_errno(mmio), "Error: %s", mmio_errmsg(mmio));

    lua_pushunsigned(L, value);

    return 1;
}

static int lua_mmio_read8(lua_State *L) {
    mmio_t *mmio;
    uint8_t value;
    uintptr_t offset;
    int ret;

    mmio = luaL_checkudata(L, 1, "periphery.MMIO");
    lua_mmio_checktype(L, 2, LUA_TNUMBER);

    offset = lua_tounsigned(L, 2);

    if ((ret = mmio_read8(mmio, offset, &value)) < 0)
        return lua_mmio_error(L, ret, mmio_errno(mmio), "Error: %s", mmio_errmsg(mmio));

    lua_pushunsigned(L, value);

    return 1;
}

static int lua_mmio_write32(lua_State *L) {
    mmio_t *mmio;
    uint32_t value;
    uintptr_t offset;
    int ret;

    mmio = luaL_checkudata(L, 1, "periphery.MMIO");
    lua_mmio_checktype(L, 2, LUA_TNUMBER);
    lua_mmio_checktype(L, 3, LUA_TNUMBER);

    offset = lua_tounsigned(L, 2);
    value = lua_tounsigned(L, 3);

    if ((ret = mmio_write32(mmio, offset, value)) < 0)
        return lua_mmio_error(L, ret, mmio_errno(mmio), "Error: %s", mmio_errmsg(mmio));

    return 0;
}

static int lua_mmio_write16(lua_State *L) {
    mmio_t *mmio;
    uint32_t value;
    uintptr_t offset;
    int ret;

    mmio = luaL_checkudata(L, 1, "periphery.MMIO");
    lua_mmio_checktype(L, 2, LUA_TNUMBER);
    lua_mmio_checktype(L, 3, LUA_TNUMBER);

    offset = lua_tounsigned(L, 2);
    value = lua_tounsigned(L, 3);

    if (value > 0xffff)
        return lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: value out of 16-bit range");

    if ((ret = mmio_write16(mmio, offset, (uint16_t)value)) < 0)
        return lua_mmio_error(L, ret, mmio_errno(mmio), "Error: %s", mmio_errmsg(mmio));

    return 0;
}

static int lua_mmio_write8(lua_State *L) {
    mmio_t *mmio;
    uint32_t value;
    uintptr_t offset;
    int ret;

    mmio = luaL_checkudata(L, 1, "periphery.MMIO");
    lua_mmio_checktype(L, 2, LUA_TNUMBER);
    lua_mmio_checktype(L, 3, LUA_TNUMBER);

    offset = lua_tounsigned(L, 2);
    value = lua_tounsigned(L, 3);

    if (value > 0xff)
        return lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: value out of 8-bit range");

    if ((ret = mmio_write8(mmio, offset, (uint8_t)value)) < 0)
        return lua_mmio_error(L, ret, mmio_errno(mmio), "Error: %s", mmio_errmsg(mmio));

    return 0;
}

static int lua_mmio_read(lua_State *L) {
    mmio_t *mmio;
    uint8_t *buf;
    uintptr_t offset;
    unsigned int i, len;
    int ret;

    mmio = luaL_checkudata(L, 1, "periphery.MMIO");
    lua_mmio_checktype(L, 2, LUA_TNUMBER);
    lua_mmio_checktype(L, 3, LUA_TNUMBER);

    offset = lua_tounsigned(L, 2);
    len = lua_tounsigned(L, 3);

    if ((buf = malloc(len)) == NULL)
        return lua_mmio_error(L, MMIO_ERROR_ALLOC, errno, "Error: allocating memory");

    if ((ret = mmio_read(mmio, offset, buf, len)) < 0) {
        free(buf);
        return lua_mmio_error(L, ret, mmio_errno(mmio), "Error: %s", mmio_errmsg(mmio));
    }

    /* Convert byte buffer to byte table */
    lua_newtable(L);
    for (i = 0; i < len; i++) {
        lua_pushunsigned(L, i+1);
        lua_pushunsigned(L, buf[i]);
        lua_settable(L, -3);
    }

    free(buf);

    return 1;
}

static int lua_mmio_write(lua_State *L) {
    mmio_t *mmio;
    uint8_t *buf;
    uintptr_t offset;
    unsigned int i, len;
    int ret;

    mmio = luaL_checkudata(L, 1, "periphery.MMIO");
    lua_mmio_checktype(L, 2, LUA_TNUMBER);
    lua_mmio_checktype(L, 3, LUA_TTABLE);

    offset = lua_tounsigned(L, 2);
    len = luaL_len(L, 3);

    if ((buf = malloc(len)) == NULL)
        return lua_mmio_error(L, MMIO_ERROR_ALLOC, errno, "Error: allocating memory");

    /* Convert byte table to byte buffer */
    for (i = 0; i < len; i++) {
        lua_pushunsigned(L, i+1);
        lua_gettable(L, -2);
        if (!lua_isnumber(L, -1)) {
            free(buf);
            return lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: invalid element index %d in bytes table.", i+1);
        }

        buf[i] = lua_tounsigned(L, -1);
        lua_pop(L, 1);
    }

    if ((ret = mmio_write(mmio, offset, buf, len)) < 0) {
        free(buf);
        return lua_mmio_error(L, ret, mmio_errno(mmio), "Error: %s", mmio_errmsg(mmio));
    }

    free(buf);

    return 0;
}

static int lua_mmio_close(lua_State *L) {
    mmio_t *mmio;
    int ret;

    mmio = luaL_checkudata(L, 1, "periphery.MMIO");

    if ((ret = mmio_close(mmio)) < 0)
        return lua_mmio_error(L, ret, mmio_errno(mmio), "Error: %s", mmio_errmsg(mmio));

    return 0;
}

static int lua_mmio_tostring(lua_State *L) {
    mmio_t *mmio;
    char mmio_str[128];

    mmio = luaL_checkudata(L, 1, "periphery.MMIO");

    mmio_tostring(mmio, mmio_str, sizeof(mmio_str));

    lua_pushstring(L, mmio_str);

    return 1;
}

static int lua_mmio_index(lua_State *L) {
    mmio_t *mmio;
    const char *field;

    if (!lua_isstring(L, 2))
        return lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: unknown method or property");

    field = lua_tostring(L, 2);

    /* Look up method in metatable */
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, field);
    if (!lua_isnil(L, -1))
        return 1;

    /* Otherwise, it's a property access */
    mmio = luaL_checkudata(L, 1, "periphery.MMIO");

    if (strcmp(field, "base") == 0) {
        lua_pushnumber(L, mmio_base(mmio));
        return 1;
    } else if (strcmp(field, "size") == 0) {
        lua_pushnumber(L, mmio_size(mmio));
        return 1;
    }

    return lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: unknown property");
}

static int lua_mmio_newindex(lua_State *L) {
    const char *field;

    if (!lua_isstring(L, 2))
        return lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: unknown property");

    field = lua_tostring(L, 2);

    if (strcmp(field, "base") == 0)
        return lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: immutable property");
    else if (strcmp(field, "size") == 0)
        return lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: immutable property");

    return lua_mmio_error(L, MMIO_ERROR_ARG, 0, "Error: unknown property");
}

static const struct luaL_Reg periphery_mmio_m[] = {
    {"close", lua_mmio_close},
    {"read32", lua_mmio_read32},
    {"read16", lua_mmio_read16},
    {"read8", lua_mmio_read8},
    {"read", lua_mmio_read},
    {"write32", lua_mmio_write32},
    {"write16", lua_mmio_write16},
    {"write8", lua_mmio_write8},
    {"write", lua_mmio_write},
    {"__gc", lua_mmio_close},
    {"__tostring", lua_mmio_tostring},
    {"__index", lua_mmio_index},
    {"__newindex", lua_mmio_newindex},
    {NULL, NULL}
};

LUALIB_API int luaopen_periphery_mmio(lua_State *L) {
    /* Create periphery.MMIO metatable */
    luaL_newmetatable(L, "periphery.MMIO");
    /* Set metatable functions */
    const struct luaL_Reg *funcs = (const struct luaL_Reg *)periphery_mmio_m;
    for (; funcs->name != NULL; funcs++) {
        lua_pushcclosure(L, funcs->func, 0);
        lua_setfield(L, -2, funcs->name);
    }
    /* Set metatable properties */
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");

    /* Create {__call = lua_mmio_new, __metatable = "protected metatable", version = ...} table */
    lua_newtable(L);
    lua_pushcclosure(L, lua_mmio_new, 0);
    lua_setfield(L, -2, "__call");
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");
    /* Set it as the metatable for the periphery.MMIO metatable */
    lua_setmetatable(L, -2);

    lua_pushstring(L, LUA_PERIPHERY_VERSION);
    lua_setfield(L, -2, "version");

    return 1;
}

