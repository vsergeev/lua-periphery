#ifndef _LUA_51COMPAT_H
#define _LUA_51COMPAT_H

#if LUA_VERSION_NUM == 501
#define lua_tounsigned(L, idx) ((uint32_t)lua_tonumber(L, idx))
#define lua_pushunsigned(L, val) (lua_pushnumber(L, (lua_Number)val))
#define luaL_len(L, idx) (lua_objlen(L, idx))
#define luaL_checkunsigned(L, narg) (luaL_checknumber(L, narg))
#endif

#endif

