#ifndef _LUA_COMPAT_H
#define _LUA_COMPAT_H

#if LUA_VERSION_NUM == 501
#define lua_tounsigned(L, idx) (lua_tonumber(L, idx))
#define lua_pushunsigned(L, val) (lua_pushnumber(L, (lua_Number)val))
#define luaL_checkunsigned(L, narg) (luaL_checknumber(L, narg))
#define luaL_len(L, idx) (lua_objlen(L, idx))
#elif LUA_VERSION_NUM == 502
#elif LUA_VERSION_NUM >= 503
#define lua_tounsigned(L, idx) (lua_tointeger(L, idx))
#define lua_pushunsigned(L, val) (lua_pushinteger(L, (lua_Integer)val))
#define luaL_checkunsigned(L, narg) (luaL_checkinteger(L, narg))
#endif

#endif

