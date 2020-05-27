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
#include "lua_compat.h"

/*
local periphery = require('periphery')
local GPIO = periphery.GPIO

-- Constructor (for character device GPIO)
gpio = GPIO(path <string>, line <number>, direction <string>)
gpio = GPIO{path=<string>, line=<number>, direction=<string>}
-- Constructor (for sysfs GPIO)
gpio = GPIO(line <number>, direction <string>)
gpio = GPIO{line=<number>, direction=<string>}

-- Methods
gpio:read() --> <boolean>
gpio:write(value <boolean>)
gpio:poll([timeout_ms <number|nil>]) --> <boolean>
gpio:close()

-- Methods (for character device GPIO)
gpio:read_event() --> {edge=<string>, timestamp=<number>}

-- Static methods
GPIO.poll_multiple(gpios <table>, [timeout_ms <number|nil>]) --> <table>

-- Properties
gpio.direction              mutable <string>
gpio.edge                   mutable <string>
gpio.line                   immutable <number>
gpio.fd                     immutable <number>
gpio.name                   immutable <number>
gpio.chip_fd                immutable <number>
gpio.chip_name              immutable <number>
gpio.chip_label             immutable <number>
*/

static const char *gpio_error_code_strings[] = {
    [-GPIO_ERROR_ARG]               = "GPIO_ERROR_ARG",
    [-GPIO_ERROR_OPEN]              = "GPIO_ERROR_OPEN",
    [-GPIO_ERROR_NOT_FOUND]         = "GPIO_ERROR_NOT_FOUND",
    [-GPIO_ERROR_QUERY]             = "GPIO_ERROR_QUERY",
    [-GPIO_ERROR_CONFIGURE]         = "GPIO_ERROR_CONFIGURE",
    [-GPIO_ERROR_UNSUPPORTED]       = "GPIO_ERROR_UNSUPPORTED",
    [-GPIO_ERROR_INVALID_OPERATION] = "GPIO_ERROR_INVALID_OPERATION",
    [-GPIO_ERROR_IO]                = "GPIO_ERROR_IO",
    [-GPIO_ERROR_CLOSE]             = "GPIO_ERROR_CLOSE",
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
    const char *path = NULL;
    unsigned int line;
    const char *line_name = NULL;
    gpio_direction_t direction;
    const char *str_direction;
    int ret;

    gpio = *((gpio_t **)luaL_checkudata(L, 1, "periphery.GPIO"));

    if (lua_istable(L, 2)) {
        /* Arguments passed in table form */

        lua_getfield(L, 2, "path");
        if (!lua_isnil(L, -1)) {
            if (lua_type(L, -1) != LUA_TSTRING)
                return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid type of table argument 'path', should be string");

            path = lua_tostring(L, -1);
        }

        lua_getfield(L, 2, "line");
        if (lua_type(L, -1) == LUA_TNUMBER)
            line = lua_tounsigned(L, -1);
        else if (path && lua_type(L, -1) == LUA_TSTRING)
            line_name = lua_tostring(L, -1);
        else
            return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid type of table argument 'line', should be number or string");

        lua_getfield(L, 2, "direction");
        if (lua_type(L, -1) != LUA_TSTRING)
            return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid type of table argument 'direction', should be string");
        str_direction = lua_tostring(L, -1);
    } else if (lua_gettop(L) > 3) {
        /* Arguments passed on stack: path (string), line (number|string), direction (string) */

        lua_gpio_checktype(L, 2, LUA_TSTRING);
        path = lua_tostring(L, 2);

        if (lua_type(L, 3) == LUA_TNUMBER)
            line = lua_tounsigned(L, 3);
        else if (lua_type(L, 3) == LUA_TSTRING)
            line_name = lua_tostring(L, 3);
        else
            return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid argument #3 (number or string expected, got %s)", lua_typename(L, lua_type(L, 3)));

        lua_gpio_checktype(L, 4, LUA_TSTRING);
        str_direction = lua_tostring(L, 4);
    } else {
        /* Arguments paseed on stack: line(number), direction (string) */

        lua_gpio_checktype(L, 2, LUA_TNUMBER);
        line = lua_tounsigned(L, 2);

        lua_gpio_checktype(L, 3, LUA_TSTRING);
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

    if (path && line_name) {
        /* character device gpio */
        if ((ret = gpio_open_name(gpio, path, line_name, direction)) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), gpio_errmsg(gpio));
    } else if (path) {
        /* character device gpio */
        if ((ret = gpio_open(gpio, path, line, direction)) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), gpio_errmsg(gpio));
    } else {
        /* sysfs gpio */
        if ((ret = gpio_open_sysfs(gpio, line, direction)) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), gpio_errmsg(gpio));
    }

    return 0;
}

static int lua_gpio_new(lua_State *L) {
    /* Remove self table object */
    lua_remove(L, 1);

    /* Create handle userdata */
    gpio_t **gpio = lua_newuserdata(L, sizeof(gpio_t *));
    *gpio = gpio_new();
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

    gpio = *((gpio_t **)luaL_checkudata(L, 1, "periphery.GPIO"));

    if ((ret = gpio_read(gpio, &value)) < 0)
        return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

    lua_pushboolean(L, value);

    return 1;
}

static int lua_gpio_write(lua_State *L) {
    gpio_t *gpio;
    bool value;
    int ret;

    gpio = *((gpio_t **)luaL_checkudata(L, 1, "periphery.GPIO"));
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

    gpio = *((gpio_t **)luaL_checkudata(L, 1, "periphery.GPIO"));

    /* Optional timeout argument */
    if (lua_isnone(L, 2) || lua_isnil(L, 2))
        timeout_ms = -1;
    else if (lua_isnumber(L, 2))
        timeout_ms = lua_tointeger(L, 2);
    else
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid type of argument 'timeout_ms', should be number or nil");

    if ((ret = gpio_poll(gpio, timeout_ms)) < 0)
        return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

    lua_pushboolean(L, ret > 0);

    return 1;
}

static int lua_gpio_read_event(lua_State *L) {
    gpio_t *gpio;
    gpio_edge_t edge;
    uint64_t timestamp;
    int ret;

    gpio = *((gpio_t **)luaL_checkudata(L, 1, "periphery.GPIO"));

    if ((ret = gpio_read_event(gpio, &edge, &timestamp)) < 0)
        return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

    lua_newtable(L);
    /* .edge string */
    switch (edge) {
        case GPIO_EDGE_NONE: lua_pushstring(L, "none"); break;
        case GPIO_EDGE_RISING: lua_pushstring(L, "rising"); break;
        case GPIO_EDGE_FALLING: lua_pushstring(L, "falling"); break;
        case GPIO_EDGE_BOTH: lua_pushstring(L, "both"); break;
        default: lua_pushstring(L, "unknown"); break;
    }
    lua_setfield(L, -2, "edge");
    /* .timestamp number */
    lua_pushunsigned(L, timestamp);
    lua_setfield(L, -2, "timestamp");

    return 1;
}

static int lua_gpio_poll_multiple(lua_State *L) {
    int ret;

    lua_gpio_checktype(L, 1, LUA_TTABLE);

    unsigned int count = luaL_len(L, 1);
    gpio_t *gpios[count];
    bool gpios_ready[count];

    /* Extract the gpio_t pointers from the input table of userdatas */
    for (unsigned int i = 0; i < count; i++) {
        /* Get the userdata from the input table */
        lua_pushunsigned(L, i+1);
        lua_gettable(L, 1);
        gpios[i] = *((gpio_t **)luaL_checkudata(L, -1, "periphery.GPIO"));
        /* Pop the userdata */
        lua_pop(L, 1);
    }

    int timeout_ms;

    /* Optional timeout argument */
    if (lua_isnone(L, 2) || lua_isnil(L, 2))
        timeout_ms = -1;
    else if (lua_isnumber(L, 2))
        timeout_ms = lua_tointeger(L, 2);
    else
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: invalid type of argument 'timeout_ms', should be number or nil");

    /* Poll */
    if ((ret = gpio_poll_multiple(gpios, count, timeout_ms, gpios_ready)) < 0)
        return lua_gpio_error(L, ret, errno, "Error: polling multiple GPIOs");

    /* Create a new output table with GPIOs that had edge events occur */
    lua_newtable(L);

    for (unsigned int i = 0, j = 1; ret && i < count; i++) {
        if (gpios_ready[i]) {
            /* Push the index into the output table */
            lua_pushunsigned(L, j++);

            /* Get the userdata from the input table */
            lua_pushunsigned(L, i+1);
            lua_gettable(L, 1);

            /* Set the user data in the output table */
            lua_settable(L, -3);
        }
    }

    return 1;
}

static int lua_gpio_close(lua_State *L) {
    gpio_t *gpio;
    int ret;

    gpio = *((gpio_t **)luaL_checkudata(L, 1, "periphery.GPIO"));

    if ((ret = gpio_close(gpio)) < 0)
        return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

    return 0;
}

static int lua_gpio_gc(lua_State *L) {
    gpio_t *gpio;

    gpio = *((gpio_t **)luaL_checkudata(L, 1, "periphery.GPIO"));

    gpio_close(gpio);

    gpio_free(gpio);

    return 0;
}

static int lua_gpio_tostring(lua_State *L) {
    gpio_t *gpio;
    char gpio_str[256];

    gpio = *((gpio_t **)luaL_checkudata(L, 1, "periphery.GPIO"));

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

    gpio = *((gpio_t **)luaL_checkudata(L, 1, "periphery.GPIO"));

    if (strcmp(field, "line") == 0) {
        lua_pushunsigned(L, gpio_line(gpio));
        return 1;
    } else if (strcmp(field, "fd") == 0) {
        lua_pushinteger(L, gpio_fd(gpio));
        return 1;
    } else if (strcmp(field, "name") == 0) {
        char name[32];
        int ret;

        if ((ret = gpio_name(gpio, name, sizeof(name))) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

        lua_pushstring(L, name);
        return 1;
    } else if (strcmp(field, "chip_fd") == 0) {
        int ret;

        if ((ret = gpio_chip_fd(gpio)) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

        lua_pushinteger(L, ret);
        return 1;
    } else if (strcmp(field, "chip_name") == 0) {
        char chip_name[32];
        int ret;

        if ((ret = gpio_chip_name(gpio, chip_name, sizeof(chip_name))) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

        lua_pushstring(L, chip_name);
        return 1;
    } else if (strcmp(field, "chip_label") == 0) {
        char chip_label[32];
        int ret;

        if ((ret = gpio_chip_label(gpio, chip_label, sizeof(chip_label))) < 0)
            return lua_gpio_error(L, ret, gpio_errno(gpio), "Error: %s", gpio_errmsg(gpio));

        lua_pushstring(L, chip_label);
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

    gpio = *((gpio_t **)luaL_checkudata(L, 1, "periphery.GPIO"));

    if (!lua_isstring(L, 2))
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: unknown property");

    field = lua_tostring(L, 2);

    if (strcmp(field, "line") == 0)
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: immutable property");
    else if (strcmp(field, "fd") == 0)
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: immutable property");
    else if (strcmp(field, "name") == 0)
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: immutable property");
    else if (strcmp(field, "chip_fd") == 0)
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: immutable property");
    else if (strcmp(field, "chip_name") == 0)
        return lua_gpio_error(L, GPIO_ERROR_ARG, 0, "Error: immutable property");
    else if (strcmp(field, "chip_label") == 0)
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
    {"read_event", lua_gpio_read_event},
    {"poll_multiple", lua_gpio_poll_multiple},
    {"__gc", lua_gpio_gc},
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

    /* Create {__call = lua_gpio_new, __metatable = "protected metatable"} table */
    lua_newtable(L);
    lua_pushcclosure(L, lua_gpio_new, 0);
    lua_setfield(L, -2, "__call");
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");
    /* Set it as the metatable for the periphery.GPIO metatable */
    lua_setmetatable(L, -2);

    return 1;
}

