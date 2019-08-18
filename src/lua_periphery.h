/*
 * lua-periphery by vsergeev
 * https://github.com/vsergeev/lua-periphery
 * License: MIT
 */

#ifndef _LUA_PERIPHERY_H
#define _LUA_PERIPHERY_H

#include <c-periphery/src/version.h>

#define _STRINGIFY(x)   #x
#define STRINGIFY(x)    _STRINGIFY(x)

#define LUA_PERIPHERY_VERSION           STRINGIFY(PERIPHERY_VERSION_MAJOR) "." \
                                        STRINGIFY(PERIPHERY_VERSION_MINOR) "." \
                                        STRINGIFY(PERIPHERY_VERSION_PATCH)

#endif

