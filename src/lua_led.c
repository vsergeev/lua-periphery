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

#include <c-periphery/src/led.h>
#include "lua_periphery.h"
#include "lua_compat.h"

/*
local periphery = require('periphery')
local LED = periphery.LED

-- Constructor
led = LED(name <string>)
led = LED{name=<string>}

-- Methods
led:read() --> <boolean>
led:write(value <boolean|number>)
led:close()

-- Properties
led.brightness      mutable <number>
led.max_brightness  immutable <number>
led.name            immutable <string>
*/

static const char *led_error_code_strings[] = {
    [-LED_ERROR_ARG]        = "LED_ERROR_ARG",
    [-LED_ERROR_OPEN]       = "LED_ERROR_OPEN",
    [-LED_ERROR_QUERY]      = "LED_ERROR_QUERY",
    [-LED_ERROR_IO]         = "LED_ERROR_IO",
    [-LED_ERROR_CLOSE]      = "LED_ERROR_CLOSE",
};

static int lua_led_error(lua_State *L, enum led_error_code code, int c_errno, const char *fmt, ...) {
    char message[128];
    va_list ap;

    va_start(ap, fmt);

    /* Create error table */
    lua_newtable(L);
    /* .code string */
    lua_pushstring(L, led_error_code_strings[-code]);
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

static void lua_led_checktype(lua_State *L, int index, int type) {
    if (lua_type(L, index) != type)
        lua_led_error(L, LED_ERROR_ARG, 0, "Error: invalid argument #%d (%s expected, got %s)", index, lua_typename(L, type), lua_typename(L, lua_type(L, index)));
}

static int lua_led_open(lua_State *L) {
    led_t *led;
    const char *name;
    int ret;

    led = *((led_t **)luaL_checkudata(L, 1, "periphery.LED"));

    /* Arguments passed in table form */
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "name");
        if (!lua_isstring(L, -1))
            return lua_led_error(L, LED_ERROR_ARG, 0, "Error: invalid type on table argument 'name', should be string");

        name = lua_tostring(L, -1);

    /* Arguments passed normally */
    } else {
        lua_led_checktype(L, 2, LUA_TSTRING);

        name = lua_tostring(L, 2);
    }

    if ((ret = led_open(led, name)) < 0)
        return lua_led_error(L, ret, led_errno(led), led_errmsg(led));

    return 0;
}

static int lua_led_new(lua_State *L) {
    /* Remove self table object */
    lua_remove(L, 1);

    /* Create handle userdata */
    led_t **led = lua_newuserdata(L, sizeof(led_t *));
    *led = led_new();
    /* Set LED metatable on it */
    luaL_getmetatable(L, "periphery.LED");
    lua_setmetatable(L, -2);
    /* Move userdata to the beginning of the stack */
    lua_insert(L, 1);

    /* Call open */
    lua_led_open(L);

    /* Leave only userdata on the stack */
    lua_settop(L, 1);

    return 1;
}

static int lua_led_read(lua_State *L) {
    led_t *led;
    bool value;
    int ret;

    led = *((led_t **)luaL_checkudata(L, 1, "periphery.LED"));

    if ((ret = led_read(led, &value)) < 0)
        return lua_led_error(L, ret, led_errno(led), "Error: %s", led_errmsg(led));

    lua_pushboolean(L, value);

    return 1;
}

static int lua_led_write(lua_State *L) {
    led_t *led;
    int ret;

    led = *((led_t **)luaL_checkudata(L, 1, "periphery.LED"));

    if (lua_isboolean(L, 2)) {
        bool value = lua_toboolean(L, 2);

        if ((ret = led_write(led, value)) < 0)
            return lua_led_error(L, ret, led_errno(led), "Error: %s", led_errmsg(led));
    } else if (lua_isnumber(L, 2)) {
        unsigned int value = lua_tounsigned(L, 2);

        if ((ret = led_set_brightness(led, value)) < 0)
            return lua_led_error(L, ret, led_errno(led), "Error: %s", led_errmsg(led));
    } else {
        return lua_led_error(L, LED_ERROR_ARG, 0, "Error: invalid value type (number or boolean expected, got %s)", lua_typename(L, lua_type(L, 2)));
    }

    return 0;
}

static int lua_led_close(lua_State *L) {
    led_t *led;
    int ret;

    led = *((led_t **)luaL_checkudata(L, 1, "periphery.LED"));

    if ((ret = led_close(led)) < 0)
        return lua_led_error(L, ret, led_errno(led), "Error: %s", led_errmsg(led));

    return 0;
}

static int lua_led_gc(lua_State *L) {
    led_t *led;

    led = *((led_t **)luaL_checkudata(L, 1, "periphery.LED"));

    led_close(led);

    led_free(led);

    return 0;
}

static int lua_led_tostring(lua_State *L) {
    led_t *led;
    char led_str[128];

    led = *((led_t **)luaL_checkudata(L, 1, "periphery.LED"));

    led_tostring(led, led_str, sizeof(led_str));

    lua_pushstring(L, led_str);

    return 1;
}

static int lua_led_index(lua_State *L) {
    led_t *led;
    const char *field;

    if (!lua_isstring(L, 2))
        return lua_led_error(L, LED_ERROR_ARG, 0, "Error: unknown method or property");

    field = lua_tostring(L, 2);

    /* Look up method in metatable */
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, field);
    if (!lua_isnil(L, -1))
        return 1;

    led = *((led_t **)luaL_checkudata(L, 1, "periphery.LED"));

    if (strcmp(field, "name") == 0) {
        char name[64];
        int ret;

        if ((ret = led_name(led, name, sizeof(name))) < 0)
            return lua_led_error(L, ret, led_errno(led), "Error: %s", led_errmsg(led));

        lua_pushstring(L, name);
        return 1;
    } else if (strcmp(field, "brightness") == 0) {
        unsigned int brightness;
        int ret;

        if ((ret = led_get_brightness(led, &brightness)) < 0)
            return lua_led_error(L, ret, led_errno(led), "Error: %s", led_errmsg(led));

        lua_pushunsigned(L, brightness);
        return 1;
    } else if (strcmp(field, "max_brightness") == 0) {
        unsigned int max_brightness;
        int ret;

        if ((ret = led_get_max_brightness(led, &max_brightness)) < 0)
            return lua_led_error(L, ret, led_errno(led), "Error: %s", led_errmsg(led));

        lua_pushunsigned(L, max_brightness);
        return 1;
    }

    return lua_led_error(L, LED_ERROR_ARG, 0, "Error: unknown property");
}

static int lua_led_newindex(lua_State *L) {
    led_t *led;
    const char *field;

    led = *((led_t **)luaL_checkudata(L, 1, "periphery.LED"));

    if (!lua_isstring(L, 2))
        return lua_led_error(L, LED_ERROR_ARG, 0, "Error: unknown property");

    field = lua_tostring(L, 2);

    if (strcmp(field, "name") == 0) {
        return lua_led_error(L, LED_ERROR_ARG, 0, "Error: immutable property");
    } else if (strcmp(field, "max_brightness") == 0) {
        return lua_led_error(L, LED_ERROR_ARG, 0, "Error: immutable property");
    } else if (strcmp(field, "brightness") == 0) {
        unsigned int brightness;
        int ret;

        lua_led_checktype(L, 3, LUA_TNUMBER);
        brightness = lua_tounsigned(L, 3);

        if ((ret = led_set_brightness(led, brightness)) < 0)
            return lua_led_error(L, ret, led_errno(led), "Error: %s", led_errmsg(led));

        return 0;
    }

    return lua_led_error(L, LED_ERROR_ARG, 0, "Error: unknown property");
}

static const struct luaL_Reg periphery_led_m[] = {
    {"close", lua_led_close},
    {"read", lua_led_read},
    {"write", lua_led_write},
    {"__gc", lua_led_gc},
    {"__tostring", lua_led_tostring},
    {"__index", lua_led_index},
    {"__newindex", lua_led_newindex},
    {NULL, NULL}
};

LUALIB_API int luaopen_periphery_led(lua_State *L) {
    /* Create periphery.LED metatable */
    luaL_newmetatable(L, "periphery.LED");
    /* Set metatable functions */
    const struct luaL_Reg *funcs = (const struct luaL_Reg *)periphery_led_m;
    for (; funcs->name != NULL; funcs++) {
        lua_pushcclosure(L, funcs->func, 0);
        lua_setfield(L, -2, funcs->name);
    }
    /* Set metatable properties */
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");

    /* Create {__call = lua_led_new, __metatable = "protected metatable"} table */
    lua_newtable(L);
    lua_pushcclosure(L, lua_led_new, 0);
    lua_setfield(L, -2, "__call");
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");
    /* Set it as the metatable for the periphery.LED metatable */
    lua_setmetatable(L, -2);

    return 1;
}

