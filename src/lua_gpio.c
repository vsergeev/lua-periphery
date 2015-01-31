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

#include <c-periphery/src/gpio.h>
#include "lua_periphery.h"
#include "lua_51compat.h"

/*
local periphery = require('periphery')
local GPIO = periphery.GPIO

-- Module Version
GPIO.version                <string>

-- Constructor
gpio = GPIO(pin <number>, direction <string>)
gpio = GPIO{pin=<number>, direction=<string>}

-- Methods
gpio:read() --> <boolean>
gpio:write(value <boolean>)
gpio:poll(timeout_ms <number>) --> <boolean>
gpio:close()

-- Properties
gpio.fd                     immutable <number>
gpio.pin                    immutable <number>
gpio.supports_interrupts    immutable <boolean>
gpio.direction              mutable <string>
gpio.edge                   mutable <string>
*/

static const char *gpio_error_code_strings[] = {
    [-GPIO_ERROR_ARG]           = "GPIO_ERROR_ARG",
    [-GPIO_ERROR_EXPORT]        = "GPIO_ERROR_EXPORT",
    [-GPIO_ERROR_OPEN]          = "GPIO_ERROR_OPEN",
    [-GPIO_ERROR_IO]            = "GPIO_ERROR_IO",
    [-GPIO_ERROR_CLOSE]         = "GPIO_ERROR_CLOSE",
    [-GPIO_ERROR_SET_DIRECTION] = "GPIO_ERROR_SET_DIRECTION",
    [-GPIO_ERROR_GET_DIRECTION] = "GPIO_ERROR_GET_DIRECTION",
    [-GPIO_ERROR_SET_EDGE]      = "GPIO_ERROR_SET_EDGE",
    [-GPIO_ERROR_GET_EDGE]      = "GPIO_ERROR_GET_EDGE",
};

static int lua_gpio_error(lua_State *L, enum gpio_error_code code, int c_errno, const char *fmt, ...) {
    char message[128];
    va_list ap;

    va_start(ap, fmt);

    /* Create error table */
    lua_newtable(L);
    /* .code string */
    lua_pushstring(L, gpio_error_code_strings[-code]);
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

static void lua_gpio_checktype(lua_State *L, int index, int type) {
    if (lua_type(L, index) != type)
        lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid argument #%d (%s expected, got %s)", index, lua_typename(L, type), lua_typename(L, lua_type(L, index)));
}

static int lua_gpio_open(lua_State *L) {
    gpio_t *gpio;
    unsigned int pin;
    gpio_direction_t direction;
    const char *str_direction;
    int ret;

    gpio = luaL_checkudata(L, 1, "periphery.GPIO");

    /* Initialize file descriptor to an invalid value, in case an error occurs
     * below and gc later close()'s this object. */
    gpio->fd = -1;

    /* Arguments passed in table form */
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "pin");
        if (!lua_isnumber(L, -1))
            return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid type of table argument 'pin', should be number");
        lua_getfield(L, 2, "direction");
        if (!lua_isstring(L, -1))
            return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid type of table argument 'direction', should be string");

        pin = lua_tounsigned(L, -2);
        str_direction = lua_tostring(L, -1);

    /* Arguments passed normally */
    } else {
        lua_gpio_checktype(L, 2, LUA_TNUMBER);
        lua_gpio_checktype(L, 3, LUA_TSTRING);

        pin = lua_tounsigned(L, 2);
        str_direction = lua_tostring(L, 3);
    }

    if (strcmp(str_direction, "in") == 0)
        direction = GPIO_DIR_IN;
    else if (strcmp(str_direction, "out") == 0)
        direction = GPIO_DIR_OUT;
    else if (strcmp(str_direction, "low") == 0)
        direction = GPIO_DIR_OUT_LOW;
    else if (strcmp(str_direction, "high") == 0)
        direction = GPIO_DIR_OUT_HIGH;
    else
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid direction, should be 'in', 'out', 'low', 'high'");

    if ((ret = gpio_open(gpio, pin, direction)) < 0)
        return lua_gpio_error(L, ret, gpio_errno(gpio), gpio_errmsg(gpio));

    return 0;
}

static int lua_gpio_new(lua_State *L) {
    /* Remove self table object */
    lua_remove(L, 1);

    /* Create handle userdata */
    lua_newuserdata(L, sizeof(gpio_t));
    /* Set GPIO metatable on it */
    luaL_getmetatable(L, "periphery.GPIO");
    lua_setmetatable(L, -2);
    /* Move userdata to the beginning of the stack */
    lua_insert(L, 1);

    /* Call open */
    lua_gpio_open(L);

    /* Leave only userdata on the stack */
    lua_settop(L, 1);

    return 1;
}

static int lua_gpio_read(lua_State *L) {
    gpio_t *gpio;
    bool value;
    int ret;

    gpio = luaL_checkudata(L, 1, "periphery.GPIO");

    if ((ret = gpio_read(gpio, &value)) < 0)
        return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

    lua_pushboolean(L, value);

    return 1;
}

static int lua_gpio_write(lua_State *L) {
    gpio_t *gpio;
    bool value;
    int ret;

    gpio = luaL_checkudata(L, 1, "periphery.GPIO");
    if (lua_isnumber(L, 2))
        value = (lua_tointeger(L, 2)) ? true : false;
    else if (lua_isboolean(L, 2))
        value = (lua_toboolean(L, 2)) ? true : false;
    else
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid value type (number or boolean expected, got %s)", lua_typename(L, lua_type(L, 2)));

    if ((ret = gpio_write(gpio, value)) < 0)
        return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

    return 0;
}

static int lua_gpio_poll(lua_State *L) {
    gpio_t *gpio;
    int timeout_ms;
    int ret;

    gpio = luaL_checkudata(L, 1, "periphery.GPIO");
    lua_gpio_checktype(L, 2, LUA_TNUMBER);

    timeout_ms = lua_tointeger(L, 2);

    if ((ret = gpio_poll(gpio, timeout_ms)) < 0)
        return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

    lua_pushboolean(L, ret > 0);

    return 1;
}

static int lua_gpio_close(lua_State *L) {
    gpio_t *gpio;
    int ret;

    gpio = luaL_checkudata(L, 1, "periphery.GPIO");

    if ((ret = gpio_close(gpio)) < 0)
        return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

    gpio->fd = -1;

    return 0;
}

static int lua_gpio_tostring(lua_State *L) {
    gpio_t *gpio;
    char gpio_str[128];

    gpio = luaL_checkudata(L, 1, "periphery.GPIO");

    gpio_tostring(gpio, gpio_str, sizeof(gpio_str));

    lua_pushstring(L, gpio_str);

    return 1;
}

static int lua_gpio_index(lua_State *L) {
    gpio_t *gpio;
    const char *field;

    if (!lua_isstring(L, 2))
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: unknown method or property");

    field = lua_tostring(L, 2);

    /* Look up method in metatable */
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, field);
    if (!lua_isnil(L, -1))
        return 1;

    gpio = luaL_checkudata(L, 1, "periphery.GPIO");

    if (strcmp(field, "fd") == 0) {
        lua_pushinteger(L, gpio_fd(gpio));
        return 1;
    } else if (strcmp(field, "pin") == 0) {
        lua_pushunsigned(L, gpio_pin(gpio));
        return 1;
    } else if (strcmp(field, "supports_interrupts") == 0) {
        bool supported;
        int ret;

        if ((ret = gpio_supports_interrupts(gpio, &supported)) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

        lua_pushboolean(L, supported);
        return 1;
    } else if (strcmp(field, "direction") == 0) {
        gpio_direction_t direction;
        int ret;

        if ((ret = gpio_get_direction(gpio, &direction)) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

        switch (direction) {
            case GPIO_DIR_IN: lua_pushstring(L, "in"); break;
            case GPIO_DIR_OUT: lua_pushstring(L, "out"); break;
            default: lua_pushstring(L, "unknown"); break;
        }
        return 1;
    } else if (strcmp(field, "edge") == 0) {
        gpio_edge_t edge;
        int ret;

        if ((ret = gpio_get_edge(gpio, &edge)) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

        switch (edge) {
            case GPIO_EDGE_NONE: lua_pushstring(L, "none"); break;
            case GPIO_EDGE_RISING: lua_pushstring(L, "rising"); break;
            case GPIO_EDGE_FALLING: lua_pushstring(L, "falling"); break;
            case GPIO_EDGE_BOTH: lua_pushstring(L, "both"); break;
            default: lua_pushstring(L, "unknown"); break;
        }
        return 1;
    }

    return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: unknown property");
}

static int lua_gpio_newindex(lua_State *L) {
    gpio_t *gpio;
    const char *field;

    gpio = luaL_checkudata(L, 1, "periphery.GPIO");

    if (!lua_isstring(L, 2))
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: unknown property");

    field = lua_tostring(L, 2);

    if (strcmp(field, "fd") == 0)
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: immutable property");
    else if (strcmp(field, "pin") == 0)
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: immutable property");
    else if (strcmp(field, "supports_interrupts") == 0)
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: immutable property");
    else if (strcmp(field, "direction") == 0) {
        gpio_direction_t direction;
        int ret;

        const char *value;
        lua_gpio_checktype(L, 3, LUA_TSTRING);
        value = lua_tostring(L, 3);

        if (strcmp(value, "in") == 0)
            direction = GPIO_DIR_IN;
        else if (strcmp(value, "out") == 0)
            direction = GPIO_DIR_OUT;
        else if (strcmp(value, "low") == 0)
            direction = GPIO_DIR_OUT_LOW;
        else if (strcmp(value, "high") == 0)
            direction = GPIO_DIR_OUT_HIGH;
        else
            return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid direction, should be 'in', 'out', 'low', or 'high'");

        if ((ret = gpio_set_direction(gpio, direction)) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

        return 0;
    } else if (strcmp(field, "edge") == 0) {
        gpio_edge_t edge;
        int ret;

        const char *value;
        lua_gpio_checktype(L, 3, LUA_TSTRING);
        value = lua_tostring(L, 3);

        if (strcmp(value, "none") == 0)
            edge = GPIO_EDGE_NONE;
        else if (strcmp(value, "rising") == 0)
            edge = GPIO_EDGE_RISING;
        else if (strcmp(value, "falling") == 0)
            edge = GPIO_EDGE_FALLING;
        else if (strcmp(value, "both") == 0)
            edge = GPIO_EDGE_BOTH;
        else
            return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid edge, should be 'none', 'rising', 'falling', or 'both'");

        if ((ret = gpio_set_edge(gpio, edge)) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

        return 0;
    }

    return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: unknown property");
}

static const struct luaL_Reg periphery_gpio_m[] = {
    {"close", lua_gpio_close},
    {"read", lua_gpio_read},
    {"write", lua_gpio_write},
    {"poll", lua_gpio_poll},
    {"__gc", lua_gpio_close},
    {"__tostring", lua_gpio_tostring},
    {"__index", lua_gpio_index},
    {"__newindex", lua_gpio_newindex},
    {NULL, NULL}
};

LUALIB_API int luaopen_periphery_gpio(lua_State *L) {
    /* Create periphery.GPIO metatable */
    luaL_newmetatable(L, "periphery.GPIO");
    /* Set metatable functions */
    const struct luaL_Reg *funcs = (const struct luaL_Reg *)periphery_gpio_m;
    for (; funcs->name != NULL; funcs++) {
        lua_pushcclosure(L, funcs->func, 0);
        lua_setfield(L, -2, funcs->name);
    }
    /* Set metatable properties */
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");

    /* Create {__call = lua_gpio_new, __metatable = "protected metatable", version = ...} table */
    lua_newtable(L);
    lua_pushcclosure(L, lua_gpio_new, 0);
    lua_setfield(L, -2, "__call");
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");
    /* Set it as the metatable for the periphery.GPIO metatable */
    lua_setmetatable(L, -2);

    lua_pushstring(L, LUA_PERIPHERY_GPIO_VERSION);
    lua_setfield(L, -2, "version");

    return 1;
}

