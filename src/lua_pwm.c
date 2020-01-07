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

#include <c-periphery/src/pwm.h>
#include "lua_periphery.h"
#include "lua_compat.h"

/*
local periphery = require('periphery')
local PWM = periphery.PWM

-- Constructor
pwm = PWM(chip <number>, channel <number>)
pwm = PWM{chip=<number>, channel=<number>}

-- Methods
pwm:enable()
pwm:disable()
pwm:close()

-- Properties
pwm.enabled         mutable <boolean>
pwm.period_ns       mutable <number>
pwm.duty_cycle_ns   mutable <number>
pwm.period          mutable <number>
pwm.duty_cycle      mutable <number>
pwm.frequency       mutable <number>
pwm.polarity        mutable <string>
pwm.chip            immutable <number>
pwm.channel         immutable <number>
*/

static const char *pwm_error_code_strings[] = {
    [-PWM_ERROR_ARG]        = "PWM_ERROR_ARG",
    [-PWM_ERROR_OPEN]       = "PWM_ERROR_OPEN",
    [-PWM_ERROR_QUERY]      = "PWM_ERROR_QUERY",
    [-PWM_ERROR_CONFIGURE]  = "PWM_ERROR_CONFIGURE",
    [-PWM_ERROR_CLOSE]      = "PWM_ERROR_CLOSE",
};

static int lua_pwm_error(lua_State *L, enum pwm_error_code code, int c_errno, const char *fmt, ...) {
    char message[128];
    va_list ap;

    va_start(ap, fmt);

    /* Create error table */
    lua_newtable(L);
    /* .code string */
    lua_pushstring(L, pwm_error_code_strings[-code]);
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

static void lua_pwm_checktype(lua_State *L, int index, int type) {
    if (lua_type(L, index) != type)
        lua_pwm_error(L, PWM_ERROR_ARG, 0, "Error: invalid argument #%d (%s expected, got %s)", index, lua_typename(L, type), lua_typename(L, lua_type(L, index)));
}

static int lua_pwm_open(lua_State *L) {
    pwm_t *pwm;
    unsigned int chip;
    unsigned int channel;
    int ret;

    pwm = *((pwm_t **)luaL_checkudata(L, 1, "periphery.PWM"));

    /* Arguments passed in table form */
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "chip");
        if (!lua_isnumber(L, -1))
            return lua_pwm_error(L, PWM_ERROR_ARG, 0, "Error: invalid type on table argument 'chip', should be number");
        lua_getfield(L, 2, "channel");
        if (!lua_isnumber(L, -1))
            return lua_pwm_error(L, PWM_ERROR_ARG, 0, "Error: invalid type on table argument 'channel', should be number");

        chip = lua_tounsigned(L, -2);
        channel = lua_tounsigned(L, -1);

    /* Arguments passed normally */
    } else {
        lua_pwm_checktype(L, 2, LUA_TNUMBER);
        lua_pwm_checktype(L, 3, LUA_TNUMBER);

        chip = lua_tounsigned(L, 2);
        channel = lua_tounsigned(L, 3);
    }

    if ((ret = pwm_open(pwm, chip, channel)) < 0)
        return lua_pwm_error(L, ret, pwm_errno(pwm), pwm_errmsg(pwm));

    return 0;
}

static int lua_pwm_new(lua_State *L) {
    /* Remove self table object */
    lua_remove(L, 1);

    /* Create handle userdata */
    pwm_t **pwm = lua_newuserdata(L, sizeof(pwm_t *));
    *pwm = pwm_new();
    /* Set PWM metatable on it */
    luaL_getmetatable(L, "periphery.PWM");
    lua_setmetatable(L, -2);
    /* Move userdata to the beginning of the stack */
    lua_insert(L, 1);

    /* Call open */
    lua_pwm_open(L);

    /* Leave only userdata on the stack */
    lua_settop(L, 1);

    return 1;
}

static int lua_pwm_enable(lua_State *L) {
    pwm_t *pwm;
    int ret;

    pwm = *((pwm_t **)luaL_checkudata(L, 1, "periphery.PWM"));

    if ((ret = pwm_enable(pwm)) < 0)
        return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

    return 0;
}

static int lua_pwm_disable(lua_State *L) {
    pwm_t *pwm;
    int ret;

    pwm = *((pwm_t **)luaL_checkudata(L, 1, "periphery.PWM"));

    if ((ret = pwm_disable(pwm)) < 0)
        return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

    return 0;
}

static int lua_pwm_close(lua_State *L) {
    pwm_t *pwm;
    int ret;

    pwm = *((pwm_t **)luaL_checkudata(L, 1, "periphery.PWM"));

    if ((ret = pwm_close(pwm)) < 0)
        return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

    return 0;
}

static int lua_pwm_gc(lua_State *L) {
    pwm_t *pwm;

    pwm = *((pwm_t **)luaL_checkudata(L, 1, "periphery.PWM"));

    pwm_close(pwm);

    pwm_free(pwm);

    return 0;
}

static int lua_pwm_tostring(lua_State *L) {
    pwm_t *pwm;
    char pwm_str[128];

    pwm = *((pwm_t **)luaL_checkudata(L, 1, "periphery.PWM"));

    pwm_tostring(pwm, pwm_str, sizeof(pwm_str));

    lua_pushstring(L, pwm_str);

    return 1;
}

static int lua_pwm_index(lua_State *L) {
    pwm_t *pwm;
    const char *field;

    if (!lua_isstring(L, 2))
        return lua_pwm_error(L, PWM_ERROR_ARG, 0, "Error: unknown method or property");

    field = lua_tostring(L, 2);

    /* Look up method in metatable */
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, field);
    if (!lua_isnil(L, -1))
        return 1;

    pwm = *((pwm_t **)luaL_checkudata(L, 1, "periphery.PWM"));

    if (strcmp(field, "chip") == 0) {
        lua_pushunsigned(L, pwm_chip(pwm));
        return 1;
    } else if (strcmp(field, "channel") == 0) {
        lua_pushunsigned(L, pwm_channel(pwm));
        return 1;
    } else if (strcmp(field, "enabled") == 0) {
        bool enabled;
        int ret;

        if ((ret = pwm_get_enabled(pwm, &enabled)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        lua_pushboolean(L, enabled);
        return 1;
    } else if (strcmp(field, "period_ns") == 0) {
        uint64_t period_ns;
        int ret;

        if ((ret = pwm_get_period_ns(pwm, &period_ns)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        lua_pushunsigned(L, period_ns);
        return 1;
    } else if (strcmp(field, "duty_cycle_ns") == 0) {
        uint64_t duty_cycle_ns;
        int ret;

        if ((ret = pwm_get_duty_cycle_ns(pwm, &duty_cycle_ns)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        lua_pushunsigned(L, duty_cycle_ns);
        return 1;
    } else if (strcmp(field, "period") == 0) {
        double period;
        int ret;

        if ((ret = pwm_get_period(pwm, &period)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        lua_pushnumber(L, period);
        return 1;
    } else if (strcmp(field, "duty_cycle") == 0) {
        double duty_cycle;
        int ret;

        if ((ret = pwm_get_duty_cycle(pwm, &duty_cycle)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        lua_pushnumber(L, duty_cycle);
        return 1;
    } else if (strcmp(field, "frequency") == 0) {
        double frequency;
        int ret;

        if ((ret = pwm_get_frequency(pwm, &frequency)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        lua_pushnumber(L, frequency);
        return 1;
    } else if (strcmp(field, "polarity") == 0) {
        pwm_polarity_t polarity;
        int ret;

        if ((ret = pwm_get_polarity(pwm, &polarity)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        switch (polarity) {
            case PWM_POLARITY_NORMAL: lua_pushstring(L, "normal"); break;
            case PWM_POLARITY_INVERSED: lua_pushstring(L, "inversed"); break;
            default: lua_pushstring(L, "unknown"); break;
        }
        return 1;
    }

    return lua_pwm_error(L, PWM_ERROR_ARG, 0, "Error: unknown property");
}

static int lua_pwm_newindex(lua_State *L) {
    pwm_t *pwm;
    const char *field;

    pwm = *((pwm_t **)luaL_checkudata(L, 1, "periphery.PWM"));

    if (!lua_isstring(L, 2))
        return lua_pwm_error(L, PWM_ERROR_ARG, 0, "Error: unknown property");

    field = lua_tostring(L, 2);

    if (strcmp(field, "chip") == 0) {
        return lua_pwm_error(L, PWM_ERROR_ARG, 0, "Error: immutable property");
    } else if (strcmp(field, "channel") == 0) {
        return lua_pwm_error(L, PWM_ERROR_ARG, 0, "Error: immutable property");
    } else if (strcmp(field, "enabled") == 0) {
        bool enabled;
        int ret;

        lua_pwm_checktype(L, 3, LUA_TBOOLEAN);
        enabled = lua_toboolean(L, 3);

        if ((ret = pwm_set_enabled(pwm, enabled)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        return 0;
    } else if (strcmp(field, "period_ns") == 0) {
        uint64_t period_ns;
        int ret;

        lua_pwm_checktype(L, 3, LUA_TNUMBER);
        period_ns = lua_tounsigned(L, 3);

        if ((ret = pwm_set_period_ns(pwm, period_ns)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        return 0;
    } else if (strcmp(field, "duty_cycle_ns") == 0) {
        uint64_t duty_cycle_ns;
        int ret;

        lua_pwm_checktype(L, 3, LUA_TNUMBER);
        duty_cycle_ns = lua_tounsigned(L, 3);

        if ((ret = pwm_set_duty_cycle_ns(pwm, duty_cycle_ns)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        return 0;
    } else if (strcmp(field, "period") == 0) {
        double period;
        int ret;

        lua_pwm_checktype(L, 3, LUA_TNUMBER);
        period = lua_tonumber(L, 3);

        if ((ret = pwm_set_period(pwm, period)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        return 0;
    } else if (strcmp(field, "duty_cycle") == 0) {
        double duty_cycle;
        int ret;

        lua_pwm_checktype(L, 3, LUA_TNUMBER);
        duty_cycle = lua_tonumber(L, 3);

        if ((ret = pwm_set_duty_cycle(pwm, duty_cycle)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        return 0;
    } else if (strcmp(field, "frequency") == 0) {
        double frequency;
        int ret;

        lua_pwm_checktype(L, 3, LUA_TNUMBER);
        frequency = lua_tonumber(L, 3);

        if ((ret = pwm_set_frequency(pwm, frequency)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        return 0;
    } else if (strcmp(field, "polarity") == 0) {
        const char *s;
        pwm_polarity_t polarity;
        int ret;

        lua_pwm_checktype(L, 3, LUA_TSTRING);
        s = lua_tostring(L, 3);

        if (strcmp(s, "normal") == 0)
            polarity = PWM_POLARITY_NORMAL;
        else if (strcmp(s, "inversed") == 0)
            polarity = PWM_POLARITY_INVERSED;
        else
            return lua_pwm_error(L, PWM_ERROR_ARG, 0, "Error: invalid polarity, should be 'normal' or 'inversed'");

        if ((ret = pwm_set_polarity(pwm, polarity)) < 0)
            return lua_pwm_error(L, ret, pwm_errno(pwm), "Error: %s", pwm_errmsg(pwm));

        return 0;
    }

    return lua_pwm_error(L, PWM_ERROR_ARG, 0, "Error: unknown property");
}

static const struct luaL_Reg periphery_pwm_m[] = {
    {"close", lua_pwm_close},
    {"enable", lua_pwm_enable},
    {"disable", lua_pwm_disable},
    {"__gc", lua_pwm_gc},
    {"__tostring", lua_pwm_tostring},
    {"__index", lua_pwm_index},
    {"__newindex", lua_pwm_newindex},
    {NULL, NULL}
};

LUALIB_API int luaopen_periphery_pwm(lua_State *L) {
    /* Create periphery.PWM metatable */
    luaL_newmetatable(L, "periphery.PWM");
    /* Set metatable functions */
    const struct luaL_Reg *funcs = (const struct luaL_Reg *)periphery_pwm_m;
    for (; funcs->name != NULL; funcs++) {
        lua_pushcclosure(L, funcs->func, 0);
        lua_setfield(L, -2, funcs->name);
    }
    /* Set metatable properties */
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");

    /* Create {__call = lua_pwm_new, __metatable = "protected metatable"} table */
    lua_newtable(L);
    lua_pushcclosure(L, lua_pwm_new, 0);
    lua_setfield(L, -2, "__call");
    lua_pushstring(L, "protected metatable");
    lua_setfield(L, -2, "__metatable");
    /* Set it as the metatable for the periphery.PWM metatable */
    lua_setmetatable(L, -2);

    return 1;
}

