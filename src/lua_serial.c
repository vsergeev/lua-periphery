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

#include <c-periphery/src/serial.h>
#include "lua_periphery.h"
#include "lua_compat.h"

/*
local periphery = require('periphery')
local Serial = periphery.Serial

-- Module Version
Serial.version      <string>

-- Constructor
serial = Serial(device <path string>, baudrate <number>)
serial = Serial{device=<path string>, baudrate=<number>, databits=8, parity="none", stopbits=1, xonxoff=false, rtscts=false}

-- Methods
serial:read(length <number>, [timeout <number>]) --> <string>
serial:read{length=<length>, timeout=nil} --> <string>
serial:write(data <string>) --> <number>
serial:poll(timeout_ms) --> <boolean>
serial:flush()
serial:input_waiting() --> <number>
serial:output_waiting() --> <number>
serial:close()

-- Properties
serial.baudrate     mutable <number>
serial.databits     mutable <number>
serial.parity       mutable <string>
serial.stopbits     mutable <number>
serial.xonxoff      mutable <boolean>
serial.rtscts       mutable <boolean>
serial.fd           immutable <number>
*/

/* Define a new error for malloc() required in read/write */
#define SERIAL_ERROR_ALLOC    (SERIAL_ERROR_CLOSE-1)

static const char *serial_error_code_strings[] = {
    [-SERIAL_ERROR_ARG]         = "SERIAL_ERROR_ARG",
    [-SERIAL_ERROR_OPEN]        = "SERIAL_ERROR_OPEN",
    [-SERIAL_ERROR_QUERY]       = "SERIAL_ERROR_QUERY",
    [-SERIAL_ERROR_IO]          = "SERIAL_ERROR_IO",
    [-SERIAL_ERROR_CONFIGURE]   = "SERIAL_ERROR_CONFIGURE",
    [-SERIAL_ERROR_CLOSE]       = "SERIAL_ERROR_CLOSE",
    [-SERIAL_ERROR_ALLOC]       = "SERIAL_ERROR_ALLOC",
};

static int lua_serial_error(lua_State *L, enum serial_error_code code, int c_errno, const char *fmt, ...) {
    char message[128];
    va_list ap;

    va_start(ap, fmt);

    /* Create error table */
    lua_newtable(L);
    /* .code string */
    lua_pushstring(L, serial_error_code_strings[-code]);
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

static void lua_serial_checktype(lua_State *L, int index, int type) {
    if (lua_type(L, index) != type)
        lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid argument #%d (%s expected, got %s)", index, lua_typename(L, type), lua_typename(L, lua_type(L, index)));
}

static int lua_serial_open(lua_State *L) {
    serial_t *serial;
    const char *device;
    uint32_t baudrate;
    int databits;
    serial_parity_t parity;
    int stopbits;
    bool xonxoff;
    bool rtscts;
    int ret;

    serial = luaL_checkudata(L, 1, "periphery.Serial");

    /* Initialize file descriptor to an invalid value, in case an error occurs
     * below and gc later close()'s this object. */
    serial->fd = -1;

    /* Default settings of optional arguments */
    databits = 8;
    parity = PARITY_NONE;
    stopbits = 1;
    xonxoff = false;
    rtscts = false;

    /* Arguments passed in table form */
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "device");
        if (!lua_isstring(L, -1))
            return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid type on table argument 'device', should be string");
        lua_getfield(L, 2, "baudrate");
        if (!lua_isnumber(L, -1))
            return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid type on table argument 'baudrate', should be number");

        device = lua_tostring(L, -2);
        baudrate = lua_tounsigned(L, -1);

        /* Optional databits */
        lua_getfield(L, 2, "databits");
        if (lua_isnumber(L, -1))
            databits = lua_tounsigned(L, -1);
        else if (!lua_isnil(L, -1))
            return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid type of table argument 'databits', should be number");

        /* Optional parity */
        lua_getfield(L, 2, "parity");
        if (lua_isstring(L, -1)) {
            const char *s = lua_tostring(L, -1);
            if (strcmp(s, "none") == 0)
                parity = PARITY_NONE;
            else if (strcmp(s, "even") == 0)
                parity = PARITY_EVEN;
            else if (strcmp(s, "odd") == 0)
                parity = PARITY_ODD;
            else
                return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid parity, should be 'none', 'even', or 'odd'");
        } else if (!lua_isnil(L, -1))
            return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid type of table argument 'parity', should be string");

        /* Optional stopbits */
        lua_getfield(L, 2, "stopbits");
        if (lua_isnumber(L, -1))
            stopbits = lua_tounsigned(L, -1);
        else if (!lua_isnil(L, -1))
            return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid type of table argument 'stopbits', should be number");

        /* Optional xonxoff */
        lua_getfield(L, 2, "xonxoff");
        if (lua_isboolean(L, -1))
            xonxoff = lua_toboolean(L, -1);
        else if (!lua_isnil(L, -1))
            return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid type of table argument 'xonxoff', should be boolean");

        /* Optional rtscts */
        lua_getfield(L, 2, "rtscts");
        if (lua_isboolean(L, -1))
            rtscts = lua_toboolean(L, -1);
        else if (!lua_isnil(L, -1))
            return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid type of table argument 'rtscts', should be boolean");

    /* Arguments passed normally */
    } else {
        lua_serial_checktype(L, 2, LUA_TSTRING);
        lua_serial_checktype(L, 3, LUA_TNUMBER);

        device = lua_tostring(L, 2);
        baudrate = lua_tounsigned(L, 3);
    }

    if ((ret = serial_open_advanced(serial, device, baudrate, databits, parity, stopbits, xonxoff, rtscts)) < 0)
        return lua_serial_error(L, ret, serial_errno(serial), serial_errmsg(serial));

    return 0;
}

static int lua_serial_new(lua_State *L) {
    /* Remove self table object */
    lua_remove(L, 1);

    /* Create handle userdata */
    lua_newuserdata(L, sizeof(serial_t));
    /* Set SERIAL metatable on it */
    luaL_getmetatable(L, "periphery.Serial");
    lua_setmetatable(L, -2);
    /* Move userdata to the beginning of the stack */
    lua_insert(L, 1);

    /* Call open */
    lua_serial_open(L);

    /* Leave only userdata on the stack */
    lua_settop(L, 1);

    return 1;
}

static int lua_serial_read(lua_State *L) {
    serial_t *serial;
    uint8_t *buf;
    size_t len;
    int timeout_ms;
    int ret;

    serial = luaL_checkudata(L, 1, "periphery.Serial");

    /* Default timeout */
    timeout_ms = -1;

    /* Arguments passed in table form */
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "length");
        if (!lua_isnumber(L, -1))
            return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid type on table argument 'length', should be number");

        len = lua_tounsigned(L, -1);

        /* Optional timeout argument */
        lua_getfield(L, 2, "timeout");
        if (lua_isnil(L, -1))
            ;
        else if (lua_isnumber(L, -1))
            timeout_ms = lua_tounsigned(L, -1);
        else
            return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid type of table argument 'timeout', should be nil or number");

    /* Arguments passed normally */
    } else {
        lua_serial_checktype(L, 2, LUA_TNUMBER);

        len = lua_tounsigned(L, 2);

        /* Optional timeout argument */
        if (lua_isnone(L, 3) || lua_isnil(L, 3))
            ;
        else if (lua_isnumber(L, 3))
            timeout_ms = lua_tounsigned(L, 3);
        else
            return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid type argument 'timeout', should be number or nil");
    }

    if ((buf = malloc(len)) == NULL)
        return lua_serial_error(L, SERIAL_ERROR_ALLOC, errno, "Error: allocating memory");

    if ((ret = serial_read(serial, buf, len, timeout_ms)) < 0) {
        free(buf);
        return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));
    }

    lua_pushlstring(L, (char *)buf, ret);

    free(buf);

    return 1;
}

static int lua_serial_write(lua_State *L) {
    serial_t *serial;
    const uint8_t *buf;
    size_t len;
    int ret;

    serial = luaL_checkudata(L, 1, "periphery.Serial");
    lua_serial_checktype(L, 2, LUA_TSTRING);

    buf = (const uint8_t *)lua_tolstring(L, 2, &len);

    if ((ret = serial_write(serial, buf, len)) < 0)
        return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

    lua_pushinteger(L, ret);
    return 1;
}

static int lua_serial_flush(lua_State *L) {
    serial_t *serial;
    int ret;

    serial = luaL_checkudata(L, 1, "periphery.Serial");

    if ((ret = serial_flush(serial)) < 0)
        return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

    return 0;
}

static int lua_serial_input_waiting(lua_State *L) {
    serial_t *serial;
    unsigned int count;
    int ret;

    serial = luaL_checkudata(L, 1, "periphery.Serial");

    if ((ret = serial_input_waiting(serial, &count)) < 0)
        return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

    lua_pushinteger(L, count);
    return 1;
}

static int lua_serial_output_waiting(lua_State *L) {
    serial_t *serial;
    unsigned int count;
    int ret;

    serial = luaL_checkudata(L, 1, "periphery.Serial");

    if ((ret = serial_output_waiting(serial, &count)) < 0)
        return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

    lua_pushinteger(L, count);

    return 1;
}

static int lua_serial_poll(lua_State *L) {
    serial_t *serial;
    int timeout_ms;
    int ret;

    serial = luaL_checkudata(L, 1, "periphery.Serial");

    lua_serial_checktype(L, 2, LUA_TNUMBER);

    timeout_ms = lua_tounsigned(L, 2);

    if ((ret = serial_poll(serial, timeout_ms)) < 0)
        return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

    lua_pushboolean(L, ret);

    return 1;
}

static int lua_serial_close(lua_State *L) {
    serial_t *serial;
    int ret;

    serial = luaL_checkudata(L, 1, "periphery.Serial");

    if ((ret = serial_close(serial)) < 0)
        return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

    return 0;
}

static int lua_serial_tostring(lua_State *L) {
    serial_t *serial;
    char serial_str[128];

    serial = luaL_checkudata(L, 1, "periphery.Serial");

    serial_tostring(serial, serial_str, sizeof(serial_str));

    lua_pushstring(L, serial_str);

    return 1;
}

static int lua_serial_index(lua_State *L) {
    serial_t *serial;
    const char *field;

    if (!lua_isstring(L, 2))
        return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: unknown method or property");

    field = lua_tostring(L, 2);

    /* Look up method in metatable */
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, field);
    if (!lua_isnil(L, -1))
        return 1;

    serial = luaL_checkudata(L, 1, "periphery.Serial");

    if (strcmp(field, "fd") == 0) {
        lua_pushinteger(L, serial_fd(serial));
        return 1;
    } else if (strcmp(field, "baudrate") == 0) {
        uint32_t baudrate;
        int ret;

        if ((ret = serial_get_baudrate(serial, &baudrate)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        lua_pushunsigned(L, baudrate);
        return 1;
    } else if (strcmp(field, "databits") == 0) {
        unsigned int databits;
        int ret;

        if ((ret = serial_get_databits(serial, &databits)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        lua_pushunsigned(L, databits);
        return 1;
    } else if (strcmp(field, "parity") == 0) {
        serial_parity_t parity;
        int ret;

        if ((ret = serial_get_parity(serial, &parity)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        switch (parity) {
            case PARITY_NONE: lua_pushstring(L, "none"); break;
            case PARITY_ODD: lua_pushstring(L, "odd"); break;
            case PARITY_EVEN: lua_pushstring(L, "even"); break;
            default: lua_pushstring(L, "unknown"); break;
        }
        return 1;
    } else if (strcmp(field, "stopbits") == 0) {
        unsigned int stopbits;
        int ret;

        if ((ret = serial_get_stopbits(serial, &stopbits)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        lua_pushunsigned(L, stopbits);
        return 1;
    } else if (strcmp(field, "xonxoff") == 0) {
        bool xonxoff;
        int ret;

        if ((ret = serial_get_xonxoff(serial, &xonxoff)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        lua_pushboolean(L, xonxoff);
        return 1;
    } else if (strcmp(field, "rtscts") == 0) {
        bool rtscts;
        int ret;

        if ((ret = serial_get_rtscts(serial, &rtscts)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        lua_pushboolean(L, rtscts);
        return 1;
    }

    return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: unknown property");
}

static int lua_serial_newindex(lua_State *L) {
    serial_t *serial;
    const char *field;

    serial = luaL_checkudata(L, 1, "periphery.Serial");

    if (!lua_isstring(L, 2))
        return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: unknown property");

    field = lua_tostring(L, 2);

    if (strcmp(field, "fd") == 0)
        return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: immutable property");
    else if (strcmp(field, "baudrate") == 0) {
        uint32_t baudrate;
        int ret;

        lua_serial_checktype(L, 3, LUA_TNUMBER);
        baudrate = lua_tounsigned(L, 3);

        if ((ret = serial_set_baudrate(serial, baudrate)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        return 0;
    } else if (strcmp(field, "databits") == 0) {
        int databits;
        int ret;

        lua_serial_checktype(L, 3, LUA_TNUMBER);
        databits = lua_tounsigned(L, 3);

        if ((ret = serial_set_databits(serial, databits)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        return 0;
    } else if (strcmp(field, "parity") == 0) {
        const char *s;
        serial_parity_t parity;
        int ret;

        lua_serial_checktype(L, 3, LUA_TSTRING);
        s = lua_tostring(L, 3);

        if (strcmp(s, "none") == 0)
            parity = PARITY_NONE;
        else if (strcmp(s, "odd") == 0)
            parity = PARITY_ODD;
        else if (strcmp(s, "even") == 0)
            parity = PARITY_EVEN;
        else
            return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: invalid parity, should be 'none', 'even', or 'odd'");

        if ((ret = serial_set_parity(serial, parity)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        return 0;
    } else if (strcmp(field, "stopbits") == 0) {
        int stopbits;
        int ret;

        lua_serial_checktype(L, 3, LUA_TNUMBER);
        stopbits = lua_tounsigned(L, 3);

        if ((ret = serial_set_stopbits(serial, stopbits)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        return 0;
    } else if (strcmp(field, "xonxoff") == 0) {
        bool xonxoff;
        int ret;

        lua_serial_checktype(L, 3, LUA_TBOOLEAN);
        xonxoff = lua_toboolean(L, 3);

        if ((ret = serial_set_xonxoff(serial, xonxoff)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        return 0;
    } else if (strcmp(field, "rtscts") == 0) {
        bool rtscts;
        int ret;

        lua_serial_checktype(L, 3, LUA_TBOOLEAN);
        rtscts = lua_toboolean(L, 3);

        if ((ret = serial_set_rtscts(serial, rtscts)) < 0)
            return lua_serial_error(L, ret, serial_errno(serial), "Error: %s", serial_errmsg(serial));

        return 0;
    }

    return lua_serial_error(L, SERIAL_ERROR_ARG, 0, "Error: unknown property");
}

static const struct luaL_Reg periphery_serial_m[] = {
    {"close", lua_serial_close},
    {"read", lua_serial_read},
    {"write", lua_serial_write},
    {"flush", lua_serial_flush},
    {"input_waiting", lua_serial_input_waiting},
    {"output_waiting", lua_serial_output_waiting},
    {"poll", lua_serial_poll},
    {"__gc", lua_serial_close},
    {"__tostring", lua_serial_tostring},
    {"__index", lua_serial_index},
    {"__newindex", lua_serial_newindex},
    {NULL, NULL}
};

LUALIB_API int luaopen_periphery_serial(lua_State *L) {
    /* Create periphery.Serial metatable */
    luaL_newmetatable(L, "periphery.Serial");
    /* Set metatable functions */
    const struct luaL_Reg *funcs = (const struct luaL_Reg *)periphery_serial_m;
    for (; funcs->name != NULL; funcs++) {
        lua_pushcclosure(L, funcs->func, 0);
        lua_setfield(L, -2, funcs->name);
    }
    /* Set metatable properties */
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");

    /* Create {__call = lua_serial_new, __metatable = "protected metatable", version = ...} table */
    lua_newtable(L);
    lua_pushcclosure(L, lua_serial_new, 0);
    lua_setfield(L, -2, "__call");
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");
    /* Set it as the metatable for the periphery.Serial metatable */
    lua_setmetatable(L, -2);

    lua_pushstring(L, LUA_PERIPHERY_VERSION);
    lua_setfield(L, -2, "version");

    return 1;
}

